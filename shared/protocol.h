#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

/* ESP-NOW protocol version */
#define PROTOCOL_VERSION 0x02U

/* Message types */
typedef enum {
    MSG_TYPE_EMERGENCY     = 0x01,
    MSG_TYPE_CLEAR         = 0x02,
    MSG_TYPE_SECURITY_ALERT = 0x10,
    MSG_TYPE_STATUS        = 0x20
} message_type_t;

/* Node identifiers */
#define NODE_ID_A 0xA0U
#define NODE_ID_B 0xB0U

/* ESP-NOW configuration */
#define ESPNOW_CHANNEL 1U
#define ESPNOW_MAX_PAYLOAD 250U

/* ESP-NOW Emergency Message (shared between Node A and Node B) */
typedef struct __attribute__((packed)) {
    uint8_t msg_type;           /* 1 = Emergency alert, 2 = Clear */
    uint8_t emergency_reasons;  /* Bitfield of emergency reasons */
    char posture[16];           /* Current posture string */
    float sensor_value;         /* Relevant sensor value */
} espnow_emergency_msg_t;

#endif /* PROTOCOL_H */
