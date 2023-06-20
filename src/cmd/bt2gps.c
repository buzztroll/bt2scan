#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>

#include "buzz_opts.h"
#include "buzz_logging.h"
#include "bt2gps_api.h"



static void sig_handler(int signum) {
    logger(BUZZ_DEBUG,"received signal %d", signum);
}

static int run_poller(bt2_gps_handle_t * gps_handle, int poll_interval, int iterations) {
    float lat;
    float lon;
    int rc;
    time_t when;

    logger(BUZZ_DEBUG, "Start the poller...");
    rc = bt2_gps_start(gps_handle, poll_interval, poll_interval / 2);
    if (rc == BT2_GPS_ERROR) {
        logger(BUZZ_ERROR, "Failed to start the poller");
        return BT2_GPS_ERROR;
    }

    for (int i = 0; i < iterations; i++) {
        rc = bt2_gps_get_last_known_location(
            gps_handle,
            &lat,
            &lon,
            &when);
        if (rc != BT2_GPS_SUCCESS) {
            logger(BUZZ_ERROR, "failed to get last known location");
        }
        else {
            printf("Lat: %f Lon: %f When: %ld\n", lat, lon, when);
        }
        sleep(1);
    }

    bt2_gps_stop(gps_handle);

    return BT2_GPS_SUCCESS;
}

int main(int argc, char ** argv) {
    int opt;
    int option_index;
    float lat;
    float lon;
    int rc;
    bt2_gps_handle_t * gps_handle;
    buzz_opts_handle_t * buzz_opts;
    char * serial_path = "/dev/serial0";
    struct sigaction sa;

    buzz_opts_init(&buzz_opts, "bt2db", "Add Bluetooth and GPS info to the database.", NULL);
    buzz_opts_add_option(buzz_opts, "log-level", 'l', 1, "<ERROR|WARN|INFO|DEBUG>");
    buzz_opts_add_option(buzz_opts, "serial", 's', 1, "Path to serial port");
    buzz_opts_add_option(buzz_opts, "poll", 'p', 1, "Check GPS poll interval (0 for none)");

    char * short_opts = buzz_opts_create_short_opts(buzz_opts);
    struct option * cli_options = buzz_opts_create_long_opts(buzz_opts);
    int poll_interval = 0;

    while ((opt = getopt_long(argc, argv, short_opts, cli_options, &option_index)) != -1) {
        switch (opt) {
        case 'l':
            set_log_level(optarg);
            break;

        case 's':
            serial_path = optarg;
            break;

        case 'p':
            poll_interval = atoi(optarg);
            break;

        case 'h':
        default:
            buzz_opts_print_usage(buzz_opts, stdout);
            buzz_opts_destroy(buzz_opts);
            return 0;
        }
    }
    buzz_opts_destroy(buzz_opts);

    memset(&sa, 0, sizeof(sa));
    sa.sa_flags = SA_NOCLDSTOP;
    sa.sa_handler = sig_handler;
    sigaction(SIGINT, &sa, NULL);

    rc = bt2_gps_init(&gps_handle, serial_path, B9600);
    if (rc != BT2_GPS_SUCCESS) {
        return 1;
    }

    if (poll_interval == 0) {
        logger(BUZZ_DEBUG, "Getting location now...");
        rc = bt2_gps_get_location_now(gps_handle, &lat, &lon);
        if (rc == BT2_GPS_ERROR) {
            logger(BUZZ_ERROR, "error reading line");
            goto err;
        }
        printf("%f,%f\n", lat, lon);
    }
    else {
        logger(BUZZ_DEBUG, "Start the poller...");
        rc = run_poller(gps_handle, poll_interval, 60);
        if (rc == BT2_GPS_ERROR) {
            logger(BUZZ_ERROR, "Failed to start the poller");
            goto err;
        }
    }

    logger(BUZZ_DEBUG, "Complete");
    bt2_gps_destroy(gps_handle);

    return 0;
err:
    bt2_gps_destroy(gps_handle);
    return 1;
}

