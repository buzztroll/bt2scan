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
#include "buzz_logging.h"
#include "buzz_opts.h"


static void sig_handler(int signum) {
    logger(BUZZ_DEBUG,"received signal %d", signum);
}

static int scan_devices(bt2_scan_t * scan_handle, int scan_time)
{
    int err = 0;
    bt2_scan_record_t record;
    time_t end_time;

    logger(BUZZ_DEBUG, "scanning for %d seconds", scan_time);
    end_time = time(NULL) + scan_time;

    int done = 0;
    while (!done && time(NULL) < end_time) {
        err = bt2_scan_read_record(scan_handle, &record);
        if (err != 0) {
            done = 1;
        }
        else {
            if (record.full_name[0] != '\0') {
                printf("%s %s\n", record.address, record.full_name);
            }
        }

    }

    return 0;
}


int main(int argc, char ** argv)
{
    int rc;
    struct sigaction sa;
    int option_index = 0;
    int opt;
    int time_to_run = 30;
    bt2_scan_t * scan_handle;
    buzz_opts_handle_t * buzz_opts;

    buzz_opts_init(&buzz_opts, "bt2scan", "Look for bluetooth le devices that are advertising nearby..", NULL);
    buzz_opts_add_option(buzz_opts, "log-level", 'l', 1, "<ERROR|WARN|INFO|DEBUG>");
    buzz_opts_add_option(buzz_opts, "scan-time", 't', 1, "The number of seconds to look for devices..");
    buzz_opts_add_option(buzz_opts, "help", 'h', 0, "Show help");

    char * short_opts = buzz_opts_create_short_opts(buzz_opts);
    struct option * cli_options = buzz_opts_create_long_opts(buzz_opts);

    while ((opt = getopt_long(argc, argv, short_opts, cli_options, &option_index)) != -1) {
        switch (opt) {
        case 't':
            time_to_run = atoi(optarg);
            break;

        case 'l':
            set_log_level(optarg);
            break;

        case 'h':
        default:
            buzz_opts_print_usage(buzz_opts, stderr);
            buzz_opts_destroy(buzz_opts);
            return 0;
        }
    }
    buzz_opts_destroy(buzz_opts);

    memset(&sa, 0, sizeof(sa));
    sa.sa_flags = SA_NOCLDSTOP;
    sa.sa_handler = sig_handler;
    sigaction(SIGINT, &sa, NULL);

    rc = bt2_scan_init(&scan_handle);
    if (rc != BT2_SCAN_SUCCESS) {
        logger(BUZZ_ERROR, "failed setup bluetooth");
        return 1;
    }

    logger(BUZZ_DEBUG, "Scanning ...");

    rc = scan_devices(scan_handle, time_to_run);
    if (rc != BT2_SCAN_SUCCESS) {
        logger(BUZZ_ERROR, "Could not receive advertising events");
    }

    bt2_scan_destroy(scan_handle);

    return 0;
}
