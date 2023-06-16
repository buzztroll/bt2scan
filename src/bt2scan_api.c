#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#include "bt2scan_api.h"
#include "buzz_logging.h"


#define BT2SCAN_FLAGS                   0x01
#define BT2SCAN_NAME_SHORT              0x08
#define BT2SCAN_NAME_COMPLETE           0x09

int bt2_scan_init(bt2_scan_t ** out_scan_handle) {
    int dev_id;
    int fd;
    int rc;
    uint8_t scan_type = 0x01;
    uint16_t interval = 8;
    uint16_t window = 8;
    uint8_t own_type = 0x00;
    uint8_t filter_policy = 0x00;
    uint8_t filter_dup = 1;
    bt2_scan_t * new_handle;
    socklen_t olen;
    struct hci_filter scan_filter;

    dev_id = hci_get_route(NULL);

    fd = hci_open_dev(dev_id);
    if (fd < 0) {
        logger(BUZZ_ERROR, "failed to open device %d", dev_id);
        return BT2_SCAN_ERROR;
    }
    new_handle = (bt2_scan_t *) calloc(1, sizeof(bt2_scan_t));
    new_handle->device_id = dev_id;
    new_handle->fd = fd;

    rc = hci_le_set_scan_parameters(fd, scan_type, interval, window,
                        own_type, filter_policy, 1000);
    if (rc < 0) {
        logger(BUZZ_ERROR, "Set scan parameters failed: %s", strerror(errno));
        goto err;
    }
    rc = hci_le_set_scan_enable(fd, 0x01, filter_dup, 1000);
    if (rc < 0) {
        logger(BUZZ_ERROR, "scan failed: %s", strerror(errno));
        goto err;
    }
    olen = sizeof(new_handle->start_filter_state);
    if (getsockopt(fd, SOL_HCI, HCI_FILTER, &new_handle->start_filter_state, &olen) < 0) {
        logger(BUZZ_ERROR, "Could not get socket options");
        goto err;
    }

    hci_filter_clear(&scan_filter);
    hci_filter_set_ptype(HCI_EVENT_PKT, &scan_filter);
    hci_filter_set_event(EVT_LE_META_EVENT, &scan_filter);

    if (setsockopt(fd, SOL_HCI, HCI_FILTER, &scan_filter, sizeof(scan_filter)) < 0) {
        logger(BUZZ_ERROR, "Could not set socket options");
        goto scan_err;
    }

    *out_scan_handle = new_handle;

    return BT2_SCAN_SUCCESS;

scan_err:

    setsockopt(new_handle->fd, SOL_HCI, HCI_FILTER, &new_handle->start_filter_state, sizeof(new_handle->start_filter_state));
err:
    hci_close_dev(fd);
    free(new_handle);
    return BT2_SCAN_ERROR;
}

int bt2_scan_destroy(bt2_scan_t * scan_handle) {

    setsockopt(scan_handle->fd, SOL_HCI, HCI_FILTER, &scan_handle->start_filter_state, sizeof(scan_handle->start_filter_state));
    hci_close_dev(scan_handle->fd);
    free(scan_handle);

    return BT2_SCAN_SUCCESS;
}

static int parse_record(bt2_scan_record_t * record_out, le_advertising_info * info) {
    uint8_t * rec_buf;
    size_t rec_buf_len;
    size_t offset;

    logger(BUZZ_INFO, "parsing a record");

    memset(record_out, '\0', sizeof(bt2_scan_record_t));

    ba2str(&info->bdaddr, record_out->address);

    rec_buf = info->data;
    rec_buf_len = info->length;

    offset = 0;
    while (offset < rec_buf_len) {
        uint8_t field_len = rec_buf[0];
        size_t name_len;
        char * buf;

        if (field_len > 0) {
            if (offset + field_len > rec_buf_len) {
                logger(BUZZ_ERROR, "Bad record. Field length exceeds the size");
                return BT2_SCAN_ERROR;
            }

            switch (rec_buf[1]) {
            case BT2SCAN_NAME_SHORT:
            case BT2SCAN_NAME_COMPLETE:
                name_len = field_len - 1;
                if (name_len > BT2_SCAN_MAX_NAME_LENGTH) {
                    name_len = BT2_SCAN_MAX_NAME_LENGTH;
                }
                if (rec_buf[1] == BT2SCAN_NAME_SHORT) {
                    buf = record_out->short_name;
                }
                else {
                    buf = record_out->full_name;
                }

                memcpy(buf, &rec_buf[2], name_len);
                break;
            case BT2SCAN_FLAGS:
                record_out->flags = rec_buf[2];
                break;
            }
        }
        offset += field_len + 1;
        rec_buf += field_len + 1;
    }

    return BT2_SCAN_SUCCESS;
}

int bt2_scan_read_record(bt2_scan_t * scan_handle, bt2_scan_record_t *out_record) {

    unsigned char buf[HCI_MAX_EVENT_SIZE];
    unsigned char *ptr;
    int len;
    int err;
    evt_le_meta_event *meta;
    le_advertising_info *info;

    len = read(scan_handle->fd, buf, sizeof(buf));
    if (len < 0) {
        return BT2_SCAN_ERROR;
    }
    /* TODO check for a complete length read */

    ptr = buf + (1 + HCI_EVENT_HDR_SIZE);
    len -= (1 + HCI_EVENT_HDR_SIZE);

    meta = (evt_le_meta_event *) ptr;

    if (meta->subevent != 0x02) {
        return BT2_SCAN_ERROR;
    }

    info = (le_advertising_info *) (meta->data + 1);
    err = parse_record(out_record, info);
    return err;
}
