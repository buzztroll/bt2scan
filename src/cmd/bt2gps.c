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
    float lat;
    float lon;
    int rc;
    bt2_gps_handle_t * gps_handle;
    
    rc = bt2_gps_init(&gps_handle, "/dev/ttyS0", B9600);
    if (rc != BT2_GPS_SUCCESS) {
        return 1;
    }

    rc = bt2_gps_get_location(gps_handle, &lat, &lon);
    if (rc == BT2_GPS_ERROR) {
        logger(BUZZ_ERROR, "error reading line");
        return 1;
    }
    printf("%f,%f\n", lat, lon);

    bt2_gps_destroy(gps_handle);

    return 0;
}

