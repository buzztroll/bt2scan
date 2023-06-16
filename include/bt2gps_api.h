#ifndef BT2_GPS_API_H
#define BT2_GPS_API_H 1

#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <time.h>

#define BT2_GPS_SUCCESS 0
#define BT2_GPS_ERROR -1

#define BT2_GPS_NOT_FOUND -3

typedef struct bt2_gps_handle_s {
    int serial_port;
    pthread_cond_t cond;
    pthread_mutex_t mutex;
    pthread_t thread_id;

    float last_known_lat;
    float last_known_lon;
    time_t update_time;

    int running;

    int error_interval;
    int interval_time;
} bt2_gps_handle_t;

int bt2_gps_init(
    bt2_gps_handle_t ** out_handle, const char * serial_path, speed_t baud);

int bt2_gps_destroy(bt2_gps_handle_t * handle);

int bt2_gps_get_location_now(
    bt2_gps_handle_t * gps_handle, float * out_lat, float * out_lon);


int bt2_gps_start(bt2_gps_handle_t * gps_handle, int interval_time, int error_interval);

int bt2_gps_stop(bt2_gps_handle_t * gps_handle);

int bt2_gps_get_last_known_location(
    bt2_gps_handle_t * gps_handle, float * out_lat, float * out_lon, time_t * when);


#endif
