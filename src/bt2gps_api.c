#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sqlite3.h>
#include <unistd.h>
#include <ctype.h>

#include "bt2gps_api.h"
#include "buzz_logging.h"

#define BT2_GPS_MAX_LINE 8192

#define SKIP_CHARS "\r\n"

#define GLL_STRING "$GPGLL,"
#define GLL_LEN 6

static int read_until_char(int fd, char * buf, size_t max_size, char term_char) {
    int done = 0;
    int n;
    size_t ndx = 0;

    while(!done) {
        n = read(fd, &buf[ndx], 1);
        if (n < 0) {
            logger(BUZZ_ERROR, "GPS error returned when reading the serial port: %s", strerror(errno));
            return BT2_GPS_NOT_FOUND;
        }
        if (buf[ndx] == term_char) {
            done = 1;
        }
        if (strchr(SKIP_CHARS, buf[ndx]) == NULL) {
            ndx = ndx + n;
            if (ndx >= max_size) {
                return BT2_GPS_NOT_FOUND;
            }
        }
    }
    return ndx;
}

static int parse_ggl(char * nmea, float * out_lat, float * out_lon) {
    char * lat_ptr;
    char * long_ptr;
    char * lat_hem_ptr;
    char * long_hem_ptr;
    char * ptr;
    float  lat_raw;
    float lon_raw;
    int lat_day;
    int lon_day;
    float  lat_min;
    float lon_min;
    float  lat;
    float lon;
    int rc;

    ptr = strchr(nmea, ',');
    if (ptr == NULL) {
        return BT2_GPS_ERROR;
    }
    lat_ptr = ptr + 1;
    ptr = strchr(lat_ptr, ',');
    if (ptr == NULL) {
        return BT2_GPS_ERROR;
    }
    *ptr = '\0';
    lat_hem_ptr = ptr + 1;
    ptr = strchr(lat_hem_ptr, ',');
    if (ptr == NULL) {
        return BT2_GPS_ERROR;
    }
    *ptr = '\0';

    long_ptr = ptr + 1;
    ptr = strchr(long_ptr, ',');
    if (ptr == NULL) {
        return BT2_GPS_ERROR;
    }
    *ptr = '\0';
    long_hem_ptr = ptr + 1;

    ptr = strchr(long_hem_ptr, ',');
    if (ptr == NULL) {
        return BT2_GPS_ERROR;
    }
    *ptr = '\0';

    rc = sscanf(lat_ptr, "%f", &lat_raw);
    if (rc <= 0) {
        return BT2_GPS_ERROR;
    }
    rc = sscanf(long_ptr, "%f", &lon_raw);
    if (rc <= 0) {
        return BT2_GPS_ERROR;
    }
    lat_day = lat_raw / 100;
    lon_day = lon_raw / 100;
    lat_min = (lat_raw - (lat_day * 100.0)) / 60.0;
    lon_min = (lon_raw - (lon_day * 100.0)) / 60.0;;
    lat = lat_min + lat_day;
    lon = lon_min + lon_day;

    if (toupper(*lat_hem_ptr) == 'S') {
        lat = -lat;
    }
    if (toupper(*long_hem_ptr) == 'W') {
        lon = -lon;
    }

    *out_lat = lat;
    *out_lon = lon;

    return BT2_GPS_SUCCESS;
}

int bt2_gps_get_location(int serial_port, float * out_lat, float * out_lon) {
   
    size_t n;
    size_t comma_ndx = 0;
    char buffer[BT2_GPS_MAX_LINE];
    int done = 0;

    while (!done) {
        n = read_until_char(serial_port, buffer, BT2_GPS_MAX_LINE, ',');
        if (n < 0) {
            logger(BUZZ_ERROR, "Failed to find the first comma");
            return n;
        }
        comma_ndx = n-1;

        n = read_until_char(serial_port, &buffer[n], BT2_GPS_MAX_LINE, '\n');
        if (n < 0) {
            logger(BUZZ_ERROR, "Failed to find the first comma");
            return n;
        }
        buffer[n+comma_ndx+1] = '\0';
        if (strncmp(buffer, GLL_STRING, comma_ndx) == 0 && comma_ndx >= GLL_LEN) {
            return parse_ggl(buffer, out_lat, out_lon);
        }
    }
    return n;
}
