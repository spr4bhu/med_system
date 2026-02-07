#ifndef MESSAGE_TYPES_H
#define MESSAGE_TYPES_H

#include <stdint.h>
#include "protocol.h"

/* Posture states */
typedef enum {
    POSTURE_LYING = 0,
    POSTURE_SITTING = 1,
    POSTURE_STANDING = 2,
    POSTURE_FALL_DETECTED = 3
} posture_t;

/* Sensor data (Node A → Cloud) */
typedef struct __attribute__((packed)) {
    float heart_rate_bpm;
    float temperature_c;
    float accel_x;
    float accel_y;
    float accel_z;
    posture_t posture;
    uint32_t timestamp_ms;
} sensor_data_t;

/* Access event (Node B → Node A → Cloud) */
typedef struct __attribute__((packed)) {
    uint32_t rfid_uid;
    uint8_t access_granted;
    uint32_t timestamp_ms;
} access_event_t;

/* Intrusion event (Node B → Node A → Cloud) */
typedef struct __attribute__((packed)) {
    uint8_t pir_triggered;
    uint32_t timestamp_ms;
} intrusion_event_t;

/* ESP-NOW packet wrapper */
typedef struct __attribute__((packed)) {
    uint8_t node_id;
    message_type_t msg_type;
    uint8_t iv[AES_IV_SIZE];
    uint8_t encrypted_payload[128];
    uint16_t crc16;
    uint32_t timestamp_ms;
} espnow_packet_t;

#endif /* MESSAGE_TYPES_H */
