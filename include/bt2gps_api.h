#ifndef BT2_GPS_API_H
#define BT2_GPS_API_H 1

#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>

#define BT2_GPS_SUCCESS 0
#define BT2_GPS_ERROR -1

#define BT2_GPS_NOT_FOUND -3

typedef struct bt2_gps_handle_s {
    int serial_port;
} bt2_gps_handle_t;

int bt2_gps_init(
    bt2_gps_handle_t ** out_handle, const char * serial_path, speed_t baud);

int bt2_gps_destroy(bt2_gps_handle_t * handle);

int bt2_gps_get_location(
    bt2_gps_handle_t * gps_handle, float * out_lat, float * out_lon);


#endif
