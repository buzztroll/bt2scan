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

#define FLAGS_LIMITED_MODE_BIT 0x01
#define FLAGS_GENERAL_MODE_BIT 0x02

#define EIR_FLAGS                   0x01
#define EIR_NAME_SHORT              0x08
#define EIR_NAME_COMPLETE           0x09

#define MAX_NAME_LENGTH 128


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

static struct option cli_options[] = {
    { "help",   0, NULL, 'h' },
    { "time", required_argument, NULL, 't' },
    { 0, 0, NULL, 0 }
};

static void print_usage(void) {
    fprintf(stderr, "buzztroll bluetooth scan is bt2scan!\n");
}

int main(int argc, char ** argv)
{
    int rc;
    struct sigaction sa;
    int option_index = 0;
    int opt;
    int time_to_run = 30;
    bt2_scan_t * scan_handle;

    while ((opt = getopt_long(argc, argv, "t:hl:", cli_options, &option_index)) != -1) {
        switch (opt) {
        case 't':
            time_to_run = atoi(optarg);
            break;

        case 'l':
            set_log_level(optarg);
            break;

        case 'h':
        default:
            print_usage();
            return 0;
        }
    }

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
}
