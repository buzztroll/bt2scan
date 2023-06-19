#ifndef BT2_DB_API_H
#define BT2_DB_API_H 1

#include <sqlite3.h>

#define BT2_DB_SUCCESS 0
#define BT2_DB_ERROR -1
#define BT2_DB_NOT_FOUND -2

typedef struct bt2_db_handle_s {
    sqlite3 * sqlite_db;
    char * db_path;
} bt2_db_handle_t;

typedef struct bt2_db_device_info_s {
    int device_id;
    char * name;
    char * address;
} bt2_db_device_info_t;

int bt2_db_init(
    bt2_db_handle_t ** out_handle,
    const char * db_path);

int bt2_db_destroy(
    bt2_db_handle_t * handle);

int bt2_db_add_device(
    bt2_db_handle_t * handle,
    const char * name,
    const char * address);

int bt2_db_find_device(
    bt2_db_handle_t * handle,
    bt2_db_device_info_t * out_device_info,
    const char * address);

int bt2_db_add_location(
    bt2_db_handle_t * handle, 
    bt2_db_device_info_t * device,
    double latitude,
    double longitude);


#endif
