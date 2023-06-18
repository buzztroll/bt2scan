#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>

#include "buzz_logging.h"
#include "bt2gps_api.h"

int main(int argc, char ** argv) {
    struct termios tty;
    float lat;
    float lon;

    int serial_port = open("/dev/ttyS0", O_RDWR);
    if (serial_port < 0) {
        printf("Error %i from open: %s\n", errno, strerror(errno));
    }

    memset(&tty, 0, sizeof(tty));
    if(tcgetattr(serial_port, &tty) != 0) {
        printf("Error %i from tcgetattr: %s\n", errno, strerror(errno));
    }
    cfsetospeed (&tty, B9600);
    

    int rc = bt2_gps_get_location(serial_port, &lat, &lon);
    if (rc == BT2_GPS_ERROR) {
        logger(BUZZ_ERROR, "error reading line");
        return 1;
    }
    printf("%f,%f\n", lat, lon);

    return 0;
}

