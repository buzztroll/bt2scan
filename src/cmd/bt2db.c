#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <sqlite3.h>

#include "buzz_logging.h"
#include "buzz_opts.h"
#include "bt2db_api.h"

#define ARG_INDEX_DB 0
#define ARG_INDEX_ADDR 1
#define ARG_INDEX_NAME 2
#define ARG_INDEX_LAST 2


int main(int argc, char ** argv)
{
    int option_index = 0;
    int opt;
    char * db_path;
    char * lat_str = NULL;
    char * long_str = NULL;
    int rc;
    double latitude;
    double longitude;
    char * name;
    char * address;
    bt2_db_handle_t * db_handle;
    bt2_db_device_info_t device_info;
    buzz_opts_handle_t * buzz_opts;
    char * pgm_args[] = {"path to db", "name", "address", NULL};

    buzz_opts_init(&buzz_opts, "bt2db", "Add Bluetooth and GPS info to the database.", pgm_args);
    buzz_opts_add_option(buzz_opts, "log-level", 'l', 1, "<ERROR|WARN|INFO|DEBUG>");
    buzz_opts_add_option(buzz_opts, "lattitude", 'x', 1, "Floating point lattitude.");
    buzz_opts_add_option(buzz_opts, "longitude", 'y', 1, "Floating point longitude.");
    buzz_opts_add_option(buzz_opts, "help", 'h', 0, "Show help");

    char * short_opts = buzz_opts_create_short_opts(buzz_opts);
    struct option * cli_options = buzz_opts_create_long_opts(buzz_opts);

    while ((opt = getopt_long(argc, argv, short_opts, cli_options, &option_index)) != -1) {
        switch (opt) {
        case 'l':
            set_log_level(optarg);
            break;

        case 'x':
            lat_str = optarg;
            break;

        case 'y':
            long_str = optarg;
            break;

        case 'h':
        default:
            buzz_opts_print_usage(buzz_opts, stdout);
            buzz_opts_destroy(buzz_opts);
            return 0;
        }
    }

    if (optind + ARG_INDEX_LAST >= argc) {
        buzz_opts_print_usage(buzz_opts, stderr);
        goto opts_error;
    }

    db_path = argv[optind+ARG_INDEX_DB];
    address = argv[optind+ARG_INDEX_ADDR];
    name = argv[optind+ARG_INDEX_NAME];

    if ((lat_str == NULL) ^ (long_str == NULL)) {
        fprintf(stderr, "lattitude and longitude must be used together: %s\n", lat_str);
        buzz_opts_print_usage(buzz_opts, stderr);
        goto opts_error;
    }

    logger(BUZZ_DEBUG, "DB is %s", db_path);

    rc = bt2_db_init(&db_handle, db_path);
    if (rc != BT2_DB_SUCCESS) {
        buzz_opts_print_usage(buzz_opts, stderr);
        goto opts_error;
    }

    rc = bt2_db_find_device(db_handle, &device_info, address);
    if (rc == BT2_DB_ERROR) {
        logger(BUZZ_ERROR, "database error");
        goto db_error;
    }
    if (rc == BT2_DB_NOT_FOUND) {
        rc = bt2_db_add_device(db_handle, name, address);
        if (rc == BT2_DB_ERROR) {
            logger(BUZZ_ERROR, "failed to add the device");
            goto db_error;
        }
        rc = bt2_db_find_device(db_handle, &device_info, address);
        if (rc == BT2_DB_ERROR) {
            logger(BUZZ_ERROR, "failed to find device after adding it");
            goto db_error;
        }
        logger(BUZZ_INFO, "Successfully added a new device");
    }
    else {
        logger(BUZZ_INFO, "The device with the address %s exist", address);
    }
    if (lat_str != NULL) {
        logger(BUZZ_DEBUG, "Adding location information");
        rc = sscanf(lat_str, "%lf", &latitude);
        if (rc != 1) {
            fprintf(stderr, "Lattitude must be a floating point: %s\n", lat_str);
            buzz_opts_print_usage(buzz_opts, stderr);
            goto db_error;
        }
        rc = sscanf(long_str, "%lf", &longitude);
        if (rc != 1) {
            fprintf(stderr, "Longitude must be a floating point: %s\n", long_str);
            buzz_opts_print_usage(buzz_opts, stderr);
            goto db_error;
        }

        rc = bt2_db_add_location(db_handle, &device_info, latitude, longitude);
        if (rc == BT2_DB_ERROR) {
            logger(BUZZ_ERROR, "add location failed");
            goto db_error;
        }

        logger(BUZZ_INFO, "Successfully added location information");
    }

    buzz_opts_destroy(buzz_opts);
    bt2_db_destroy(db_handle);

    return 0;

db_error:
    bt2_db_destroy(db_handle);
opts_error:
    buzz_opts_destroy(buzz_opts);

    return 1;
}
