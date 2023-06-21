#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sqlite3.h>

#include "bt2db_api.h"
#include "buzz_logging.h"

int bt2_db_init(
    bt2_db_handle_t ** out_handle,
    const char * db_path) {
    bt2_db_handle_t * new_handle;
    int rc;
    sqlite3 * sqlite_db;

    rc = sqlite3_open(db_path, &sqlite_db);
    if(rc != SQLITE_OK) {
        logger(BUZZ_ERROR, "The database failed to open");
        return 1;
    }

    new_handle = (bt2_db_handle_t *) calloc(1, sizeof(bt2_db_handle_t));
    new_handle->sqlite_db = sqlite_db;
    *out_handle = new_handle;

    return BT2_DB_SUCCESS;
}

int bt2_db_destroy(
    bt2_db_handle_t * handle) {

    sqlite3_close(handle->sqlite_db);

    free(handle);
    return BT2_DB_SUCCESS;
}

int bt2_db_add_device(
    bt2_db_handle_t * handle,
    const char * name,
    const char * address) {
    static const char * sql_q = "INSERT into devices(name, address) values (?, ?)";
    int rc;
    sqlite3_stmt *stmt;

    rc = sqlite3_prepare_v2(handle->sqlite_db, sql_q, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
       logger(BUZZ_ERROR, "sql prepare failed: %s", sqlite3_errmsg(handle->sqlite_db));
       return BT2_DB_ERROR;
    }

    sqlite3_bind_text(stmt, 1, name, strlen(name), NULL);
    sqlite3_bind_text(stmt, 2, address, strlen(address), NULL);
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
       logger(BUZZ_ERROR, "sql insert step failed: %s", sqlite3_errmsg(handle->sqlite_db));
        goto error;
    }

    sqlite3_finalize(stmt);
    return BT2_DB_SUCCESS;

error:
    logger(BUZZ_ERROR, "bt2_db_add_device failed");
    sqlite3_finalize(stmt);
    return BT2_DB_ERROR;

}

int bt2_db_find_device(
    bt2_db_handle_t * handle,
    bt2_db_device_info_t * out_device_info,
    const char * address) {

    static const char * sql_q = "SELECT id, address, name, update_location from devices where address = ?";
    sqlite3_stmt *stmt;
    int rc;
    int id = BT2_DB_NOT_FOUND;

    rc = sqlite3_prepare_v2(handle->sqlite_db, sql_q, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
       logger(BUZZ_ERROR, "sql prepare failed: %s", sqlite3_errmsg(handle->sqlite_db));
       return BT2_DB_ERROR;
    }

    sqlite3_bind_text(stmt, 1, address, strlen(address), NULL);
    while (sqlite3_step(stmt) != SQLITE_DONE) {
        if (id != BT2_DB_NOT_FOUND) {
            logger(BUZZ_ERROR, "we found the address %s multiple times", address);
            goto error;
        }
        id = sqlite3_column_int(stmt, 0);
        out_device_info->address = sqlite3_column_text(stmt, 1);
        out_device_info->name = sqlite3_column_text(stmt, 2);
        out_device_info->update = sqlite3_column_int(stmt, 3);

        out_device_info->device_id = id;
        logger(BUZZ_DEBUG, "Found id %d", id);
    }
    sqlite3_finalize(stmt);
    return id;

error:
    logger(BUZZ_ERROR, "bt2_db_find_device failed");
    sqlite3_finalize(stmt);
    return BT2_DB_ERROR;
}

int bt2_db_add_location(
    bt2_db_handle_t * handle,
    bt2_db_device_info_t * device,
    double latitude,
    double longitude) {
    static const char * sql_q = "INSERT into locations(device_id, lat, long) values (?, ?, ?)";
    int rc;
    sqlite3_stmt *stmt;

    rc = sqlite3_prepare_v2(handle->sqlite_db, sql_q, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
       logger(BUZZ_ERROR, "sql prepare failed: %s", sqlite3_errmsg(handle->sqlite_db));
       return BT2_DB_ERROR;
    }

    sqlite3_bind_int(stmt, 1, device->device_id);
    sqlite3_bind_double(stmt, 2, latitude);
    sqlite3_bind_double(stmt, 3, longitude);
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
       logger(BUZZ_ERROR, "sql insert step failed: %s", sqlite3_errmsg(handle->sqlite_db));
        goto error;
    }

    sqlite3_finalize(stmt);
    return BT2_DB_SUCCESS;

error:
    logger(BUZZ_ERROR, "bt2_db_add_location failed");
    sqlite3_finalize(stmt);
    return BT2_DB_ERROR;
}

