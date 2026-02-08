#ifndef DATA_STRUCTURES_H
#define DATA_STRUCTURES_H

#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"

/* ========== Global FreeRTOS objects (defined in main.c) ========== */
extern QueueHandle_t sensor_data_queue;
extern SemaphoreHandle_t i2c_mutex;
extern EventGroupHandle_t emergency_event_group;

/* ========== Event group bits ========== */
#define FALL_DETECTED_BIT    (1 << 0)
#define VITALS_ABNORMAL_BIT  (1 << 1)
#define SOS_PRESSED_BIT      (1 << 2)

/* ========== MPU6050 Data ========== */
typedef struct {
    float accel_x;      /* m/s^2 */
    float accel_y;
    float accel_z;
    float gyro_x;       /* deg/s */
    float gyro_y;
    float gyro_z;
    float temperature;  /* Celsius */
} mpu6050_data_t;

/* ========== Angle Data (Complementary Filter) ========== */
typedef struct {
    float roll;
    float pitch;
} angle_t;

/* ========== Posture Detection ========== */
typedef enum {
    POSTURE_UNKNOWN  = 0,
    POSTURE_STANDING = 1,
    POSTURE_SITTING  = 2,
    POSTURE_LYING    = 3
} posture_state_t;

typedef struct {
    posture_state_t current_state;
    posture_state_t previous_state;
    uint8_t state_stable_count;
} posture_detector_t;

/* ========== Fall Detection Result ========== */
typedef struct {
    uint8_t fall_detected;
    float jerk_magnitude;
} fall_detection_result_t;

/* ========== DHT11 Data ========== */
typedef struct {
    float temperature;      /* Celsius */
    uint8_t humidity;       /* 0-100% */
    uint8_t crc_ok;         /* 1 if checksum passed */
    uint32_t last_read_ms;
} dht11_data_t;

/* ========== HW-827 Data ========== */
typedef struct {
    uint16_t adc_raw;       /* Raw ADC value (0-4095) */
    float voltage_mv;       /* Voltage in millivolts */
    uint8_t peak_detected;  /* 1 if peak detected */
    uint32_t peak_count;    /* Number of peaks */
} hw827_data_t;

/* ========== Combined Sensor Data ========== */
typedef struct {
    dht11_data_t temperature;
    hw827_data_t heart_rate;
    uint32_t last_update_ms;
} sensor_data_t;

/* ========== System State ========== */
typedef enum {
    SYSTEM_STATE_IDLE      = 0,   /* Lying down, sleeping */
    SYSTEM_STATE_NORMAL    = 1,   /* Active monitoring */
    SYSTEM_STATE_EMERGENCY = 2    /* Emergency triggered */
} system_state_t;

/* ========== Emergency Reasons (bitfield) ========== */
typedef enum {
    EMERGENCY_NONE      = 0,
    EMERGENCY_FALL      = (1 << 0),
    EMERGENCY_BUTTON    = (1 << 1),
    EMERGENCY_HIGH_TEMP = (1 << 2),
    EMERGENCY_HIGH_HR   = (1 << 3)
} emergency_reason_t;

/* ========== ESP-NOW Emergency Message ========== */
typedef struct {
    uint8_t msg_type;           /* 1 = Emergency alert, 2 = Clear */
    uint8_t emergency_reasons;  /* Bitfield of emergency_reason_t */
    char posture[16];           /* Current posture string */
    float sensor_value;         /* Relevant sensor value */
} espnow_emergency_msg_t;

#endif /* DATA_STRUCTURES_H */
