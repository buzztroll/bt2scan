#ifndef BT2_SCAN_API_H
#define BT2_SCAN_API_H 1

#include <stdint.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

#define BT2_SCAN_MAX_NAME_LENGTH 128

#define BT2_SCAN_SUCCESS 0
#define BT2_SCAN_ERROR -1

typedef struct bt2_scan_s {
    int device_id;
    int fd;
    struct hci_filter start_filter_state;
} bt2_scan_t;

typedef struct bt2_scan_record_s {
    char short_name[BT2_SCAN_MAX_NAME_LENGTH];
    char full_name[BT2_SCAN_MAX_NAME_LENGTH];
    char address[18];
    uint8_t flags;
} bt2_scan_record_t;



int bt2_scan_init(bt2_scan_t ** out_scan_handle);
int bt2_scan_destroy(bt2_scan_t * scan_handle);

int bt2_scan_read_record(bt2_scan_t * scan_handle, bt2_scan_record_t *out_record);


#endif
