#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>

#include "buzz_opts.h"
#include "buzz_logging.h"
#include "bt2gps_api.h"

int main(int argc, char ** argv) {
    int opt;
    int option_index;
    float lat;
    float lon;
    int rc;
    bt2_gps_handle_t * gps_handle;
    buzz_opts_handle_t * buzz_opts;
    char * serial_path = "/dev/serial0";

    buzz_opts_init(&buzz_opts, "bt2db", "Add Bluetooth and GPS info to the database.", NULL);
    buzz_opts_add_option(buzz_opts, "log-level", 'l', 1, "<ERROR|WARN|INFO|DEBUG>");
    buzz_opts_add_option(buzz_opts, "serial", 's', 1, "Path to serial port");

    char * short_opts = buzz_opts_create_short_opts(buzz_opts);
    struct option * cli_options = buzz_opts_create_long_opts(buzz_opts);

    while ((opt = getopt_long(argc, argv, short_opts, cli_options, &option_index)) != -1) {
        switch (opt) {
        case 'l':
            set_log_level(optarg);
            break;

        case 's':
            serial_path = optarg;
            break;

        case 'h':
        default:
            buzz_opts_print_usage(buzz_opts, stdout);
            buzz_opts_destroy(buzz_opts);
            return 0;
        }
    }
    buzz_opts_destroy(buzz_opts);

    rc = bt2_gps_init(&gps_handle, serial_path, B9600);
    if (rc != BT2_GPS_SUCCESS) {
        return 1;
    }

    logger(BUZZ_DEBUG, "Getting location...");
    rc = bt2_gps_get_location(gps_handle, &lat, &lon);
    if (rc == BT2_GPS_ERROR) {
        logger(BUZZ_ERROR, "error reading line");
        goto err;
    }
    printf("%f,%f\n", lat, lon);

    bt2_gps_destroy(gps_handle);

    return 0;
err:
    bt2_gps_destroy(gps_handle);
    return 1;
}

