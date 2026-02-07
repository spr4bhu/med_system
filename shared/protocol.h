#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

/* ESP-NOW protocol version */
#define PROTOCOL_VERSION 0x01U

/* Message types */
typedef enum {
    MSG_TYPE_SENSOR_DATA = 0x01,
    MSG_TYPE_FALL_ALERT = 0x02,
    MSG_TYPE_SOS = 0x03,
    MSG_TYPE_ACCESS_LOG = 0x10,
    MSG_TYPE_INTRUSION = 0x11,
    MSG_TYPE_VITALS_ABNORMAL = 0x20
} message_type_t;

/* Node identifiers */
#define NODE_ID_A 0xA0U
#define NODE_ID_B 0xB0U

/* ESP-NOW configuration */
#define ESPNOW_CHANNEL 1U
#define ESPNOW_MAX_PAYLOAD 250U
#define AES_IV_SIZE 16U
#define AES_KEY_SIZE 16U

#endif /* PROTOCOL_H */
