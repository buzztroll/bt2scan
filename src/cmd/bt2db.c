#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <sqlite3.h>

#include "buzz_logging.h"
#include "bt2db_api.h"

#define ARG_INDEX_DB 0
#define ARG_INDEX_ADDR 1
#define ARG_INDEX_NAME 2
#define ARG_INDEX_LAST 2


static struct option cli_options[] = {
    { "help",   0, NULL, 'h' },
    { "log-level",   required_argument, NULL, 'l' },
    { "latitude",   required_argument, NULL, 'x' },
    { "longitude",   required_argument, NULL, 'y' },
    { 0, 0, NULL, 0 }
};

static void print_usage(void) {
    fprintf(stderr, "bt2db [options] <path to db> <name> <address>\n");
    fprintf(stderr, "-----\n");
    fprintf(stderr, "Add Bluetooth and GPS info to the database.\n");
    fprintf(stderr, "If the -x and -y options are used add location info also\n");
    fprintf(stderr, "-x|--latitude <latitude>\n");
    fprintf(stderr, "-y|--longitude <longitude>\n");
    fprintf(stderr, "-l|--log-level <ERROR|WARN|INFO|DEBUG>\n");
}

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

    while ((opt = getopt_long(argc, argv, "d:hl:x:y:", cli_options, &option_index)) != -1) {
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
            print_usage();
            return 0;
        }
    }

    if (optind + ARG_INDEX_LAST >= argc) {
        print_usage();
        return 1;
    }

    db_path = argv[optind+ARG_INDEX_DB];
    address = argv[optind+ARG_INDEX_ADDR];
    name = argv[optind+ARG_INDEX_NAME];

    if ((lat_str == NULL) ^ (long_str == NULL)) {
        fprintf(stderr, "lattitude and longitude must be used together: %s\n", lat_str);
        print_usage();
        return 1;
    }

    logger(BUZZ_DEBUG, "DB is %s", db_path);

    rc = bt2_db_init(&db_handle, db_path);
    if (rc != BT2_DB_SUCCESS) {
        print_usage();
        return 1;
    }

    rc = bt2_db_find_device(db_handle, &device_info, address);
    if (rc == BT2_DB_ERROR) {
        logger(BUZZ_ERROR, "database error");
        return 1;
    }
    if (rc == BT2_DB_NOT_FOUND) {
        rc = bt2_db_add_device(db_handle, name, address);
        if (rc == BT2_DB_ERROR) {
            logger(BUZZ_ERROR, "failed to add the device");
            return 1;
        }
        rc = bt2_db_find_device(db_handle, &device_info, address);
        if (rc == BT2_DB_ERROR) {
            logger(BUZZ_ERROR, "failed to find device after adding it");
            return 1;
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
            print_usage();
            return 1;
        }
        rc = sscanf(long_str, "%lf", &longitude);
        if (rc != 1) {
            fprintf(stderr, "Longitude must be a floating point: %s\n", long_str);
            print_usage();
            return 1;
        }

        rc = bt2_db_add_location(db_handle, &device_info, latitude, longitude);
        if (rc == BT2_DB_ERROR) {
            logger(BUZZ_ERROR, "add location failed");
            return 1;
        }

        logger(BUZZ_INFO, "Successfully added location information");
    }
    bt2_db_destroy(db_handle);

    return 0;
}
