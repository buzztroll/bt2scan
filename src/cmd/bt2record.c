#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <getopt.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

#include "bt2scan_api.h"
#include "bt2gps_api.h"
#include "bt2db_api.h"
#include "buzz_logging.h"
#include "buzz_opts.h"

#define ARG_INDEX_DB 0
#define ARG_INDEX_LAST 0

static int _g_canceled = 0;

static void sig_handler(int signum) {
    logger(BUZZ_INFO,"received signal %d %d %d", signum, SIGINT, SIGTERM);
    _g_canceled = 1;
}

static int add_record(
    bt2_db_handle_t * db_handle,
    bt2_scan_record_t * record,
    float latitude,
    float longitude,
    int add_location) {
    bt2_db_device_info_t device_info;
    int rc;

    logger(BUZZ_DEBUG, "Bluetooth record found");
    rc = bt2_db_find_device(db_handle, &device_info, record->address);
    if (rc == BT2_DB_ERROR) {
        logger(BUZZ_ERROR, "database error");
        return 1;
    }
    if (rc == BT2_DB_NOT_FOUND) {
        logger(BUZZ_DEBUG, "A new device was found, adding: %s", record->full_name);
        rc = bt2_db_add_device(db_handle, record->full_name, record->address);
        if (rc == BT2_DB_ERROR) {
            logger(BUZZ_ERROR, "failed to add the device");
            return 1;
        }
        rc = bt2_db_find_device(db_handle, &device_info, record->address);
        if (rc == BT2_DB_ERROR) {
            logger(BUZZ_ERROR, "failed to find device after adding it");
            return 1;
        }
        logger(BUZZ_INFO, "Successfully added a new device");
    }
    if (device_info.update && add_location) {
        logger(BUZZ_INFO, "Adding location info for %s", record->full_name);
        rc = bt2_db_add_location(db_handle, &device_info, latitude, longitude);
        if (rc == BT2_DB_ERROR) {
            logger(BUZZ_ERROR, "add location failed");
            return 1;
        }
    }
    else {
        logger(BUZZ_INFO, "Skipping location info for %s", record->full_name);
    }
    return 0;
}

static int scan_devices(
    bt2_db_handle_t * db_handle,
    bt2_gps_handle_t * gps_handle,
    int scan_time)
{
    int rc = 0;
    bt2_scan_record_t record;
    bt2_scan_t * scan_handle;
    time_t end_time;
    time_t when;
    int add_location;

    /* open and close the BT device on every iteration */
    rc = bt2_scan_init(&scan_handle);
    if (rc != BT2_SCAN_SUCCESS) {
        logger(BUZZ_ERROR, "failed setup bluetooth");
        goto init_err;
    }

    logger(BUZZ_DEBUG, "scanning for %d seconds", scan_time);
    end_time = time(NULL) + scan_time;

    int done = 0;
    while (!done && time(NULL) < end_time) {
        rc = bt2_scan_read_record(scan_handle, &record);
        if (rc != BT2_SCAN_SUCCESS) {
            done = 1;
        }
        else {
            logger(BUZZ_DEBUG, "record found |%s| |%s|", record.full_name, record.short_name);
            if (record.full_name[0] != '\0') {
                float lat;
                float lon;

                logger(BUZZ_INFO, "Found a record: %s", record.full_name);

                add_location = 1;
                rc = bt2_gps_get_last_known_location(gps_handle, &lat, &lon, &when);
                if (rc != BT2_GPS_SUCCESS) {
                    logger(BUZZ_ERROR, "Failed to get location, adding the device info without location");
                    add_location = 0;
                }

                logger(BUZZ_INFO, "Found record to db with lat %f lon %f", lat, lon);
                rc = add_record(db_handle, &record, lat, lon, add_location);
                if (rc != BT2_GPS_SUCCESS) {
                    logger(BUZZ_ERROR, "failed to add record");
                    goto scan_err;
                }
            }
        }
    }

    bt2_scan_destroy(scan_handle);
    return 0;

scan_err:
    bt2_scan_destroy(scan_handle);
init_err:
    return -1;
}

int main(int argc, char ** argv)
{
    int rc;
    struct sigaction sa;
    int option_index = 0;
    int opt;
    int scan_time = 30;
    int delay = 60;
    buzz_opts_handle_t * buzz_opts;
    char * pgm_args[] = {"path to db", NULL};
    char * serial = "/dev/serial0";
    int done = 0;
    char * db_path;
    bt2_db_handle_t * db_handle;
    bt2_gps_handle_t * gps_handle;

    buzz_opts_init(&buzz_opts, "bt2record", "Periodically scan for LE Bluetooth devices and record the GPS coordinates when they are found", pgm_args);
    buzz_opts_add_option(buzz_opts, "log-level", 'l', 1, "<ERROR|WARN|INFO|DEBUG>");
    buzz_opts_add_option(buzz_opts, "delay", 'd', 1, "The number of seconds to wait between scans");
    buzz_opts_add_option(buzz_opts, "serial", 's', 1, "The path to the serial port device for the GPS");
    buzz_opts_add_option(buzz_opts, "scan-time", 't', 1, "The number of seconds to allow each scan to run");
    buzz_opts_add_option(buzz_opts, "help", 'h', 0, "Show help");

    char * short_opts = buzz_opts_create_short_opts(buzz_opts);
    struct option * cli_options = buzz_opts_create_long_opts(buzz_opts);

    while ((opt = getopt_long(argc, argv, short_opts, cli_options, &option_index)) != -1) {
        switch (opt) {
        case 't':
            scan_time = atoi(optarg);
            break;

        case 'l':
            set_log_level(optarg);
            break;

        case 'd':
            delay = atoi(optarg);
            break;

        case 's':
            serial = optarg;
            break;

        case 'h':
        default:
            buzz_opts_print_usage(buzz_opts, stderr);
            buzz_opts_destroy(buzz_opts);
            return 0;
        }
    }
    if (optind + ARG_INDEX_LAST >= argc) {
        buzz_opts_print_usage(buzz_opts, stderr);
        buzz_opts_destroy(buzz_opts);
        return 1;
    }
    buzz_opts_destroy(buzz_opts);
    db_path = argv[optind+ARG_INDEX_DB];

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sig_handler;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    rc = bt2_db_init(&db_handle, db_path);
    if (rc != BT2_DB_SUCCESS) {
        logger(BUZZ_ERROR, "failed setup database %s", db_path);
        goto db_init_err;;
    }

    rc = bt2_gps_init(&gps_handle, serial, B9600);
    if (rc != BT2_GPS_SUCCESS) {
        logger(BUZZ_ERROR, "failed setup gps: %s", serial);
        goto gps_init_err;
    }

    rc = bt2_gps_start(gps_handle, delay/2, delay/2);
    if (rc != BT2_GPS_SUCCESS) {
        logger(BUZZ_ERROR, "failed start gps: %s", serial);
        goto gps_start_err;
    }

    while (!done) {
        logger(BUZZ_DEBUG, "starting a pass...");

        rc = scan_devices(db_handle, gps_handle, scan_time);
        if (rc != 0) {
            logger(BUZZ_ERROR, "scan_devices failed");
            goto gps_running_err;
        }
        logger(BUZZ_DEBUG, "Waiting %ds for next scan", delay);
        if (!_g_canceled) {
            sleep(delay);
        }
        done = _g_canceled;
    }
    bt2_gps_stop(gps_handle);
    logger(BUZZ_DEBUG, "complete");
    bt2_gps_destroy(gps_handle);
    bt2_db_destroy(db_handle);
    return 0;

gps_running_err:
    bt2_gps_stop(gps_handle);
gps_start_err:
    bt2_gps_destroy(gps_handle);
gps_init_err:
    bt2_db_destroy(db_handle);
db_init_err:
    return 1;
}
