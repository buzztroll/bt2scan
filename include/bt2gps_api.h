#ifndef BT2_GPS_API_H
#define BT2_GPS_API_H 1

#define BT2_GPS_SUCCESS 0
#define BT2_GPS_ERROR -1

#define BT2_GPS_NOT_FOUND -3

typedef struct bt2_gps_handle_s {
    int x;
} bt2_gps_handle_t;

int bt2_gps_get_location(int serial_port, float * out_lat, float * out_lon);


#endif
