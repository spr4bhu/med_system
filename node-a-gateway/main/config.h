/* Configuration file - Replace placeholder values with your credentials */

#ifndef CONFIG_H
#define CONFIG_H

#include "driver/i2c.h"
#include "esp_adc/adc_oneshot.h"

/* ========== WiFi Credentials ========== */
#define WIFI_SSID     "YOUR_WIFI_SSID"
#define WIFI_PASS     "YOUR_WIFI_PASSWORD"

/* ========== MQTT Configuration (HiveMQ Cloud TLS) ========== */
#define MQTT_BROKER_URI "mqtts://YOUR_BROKER.s1.eu.hivemq.cloud:8883"
#define MQTT_USERNAME   "YOUR_MQTT_USERNAME"
#define MQTT_PASSWORD   "YOUR_MQTT_PASSWORD"
#define USE_HIVEMQ_CLOUD 1  /* Set to 0 for local MQTT */

/* ========== SMS / IFTTT Alert Configuration ========== */
#define USE_TWILIO 0
#define USE_IFTTT  0
#define IFTTT_EVENT_NAME "esp32_emergency"
#define IFTTT_KEY        "YOUR_IFTTT_KEY_HERE"

/* ========== ESP-NOW Configuration ========== */
#define ESP_NOW_CHANNEL  1

/* ========== GPIO Pin Configuration - Node A ========== */
#define I2C_MASTER_SDA_IO    21
#define I2C_MASTER_SCL_IO    22
#define I2C_MASTER_NUM       I2C_NUM_0
#define I2C_MASTER_FREQ_HZ   100000

#define EMERGENCY_BUTTON_PIN 4   /* GPIO4 - external 2.2k pull-up */
#define DHT11_GPIO_PIN       5   /* GPIO5 - DHT11 1-wire */
#define HW827_ADC_CHANNEL    ADC_CHANNEL_0  /* GPIO36 */
#define HW827_ADC_UNIT       ADC_UNIT_1

/* ========== I2C Device Addresses ========== */
#define MPU6050_I2C_ADDR     0x68U

/* ========== MPU6050 Registers ========== */
#define MPU6050_PWR_MGMT_1   0x6B
#define MPU6050_SMPLRT_DIV   0x19
#define MPU6050_CONFIG_REG   0x1A
#define MPU6050_GYRO_CONFIG  0x1B
#define MPU6050_ACCEL_CONFIG 0x1C
#define MPU6050_ACCEL_XOUT_H 0x3B

/* ========== MPU6050 Scaling Constants ========== */
#define ACCEL_SCALE_FACTOR   8192.0f  /* +/-4g range */
#define GYRO_SCALE_FACTOR    65.5f    /* +/-500 deg/s range */
#define GRAVITY              9.81f

/* ========== Fall Detection Parameters ========== */
#define FALL_JERK_THRESHOLD  200.0f   /* g/s threshold */
#define FALL_COOLDOWN_MS     2000     /* ms cooldown after fall */
#define SAMPLE_INTERVAL_MS   10       /* 100Hz sampling */
#define SAMPLE_INTERVAL_SEC  0.01f

/* ========== Posture Detection Parameters ========== */
#define ALPHA                0.99f        /* Gyro weight in complementary filter */
#define RAD_TO_DEG           57.27272727f
#define SITTING_PITCH_MIN    20.0f    /* Min pitch for sitting (degrees) */
#define STANDING_PITCH_MAX   15.0f    /* Max pitch for standing (degrees) */
#define LYING_PITCH_MAX      10.0f    /* Max pitch for lying (degrees) */
#define HYSTERESIS_COUNT     50       /* Stable samples before transition */
#define POSTURE_PUBLISH_DELAY_MS 2000

/* ========== DHT11 Sensor Configuration ========== */
#define DHT11_READ_INTERVAL_MS  2000    /* Minimum 2s (DHT11 spec) */
#define TEMP_ALERT_THRESHOLD    30.0f   /* Alert if temp >= 30C */

/* ========== HW-827 Sensor Configuration ========== */
#define HW827_SAMPLE_INTERVAL   50      /* 20Hz (50ms) */
#define HR_ALERT_THRESHOLD      120     /* Alert if BPM >= 120 */

/* ========== Task Intervals (milliseconds) ========== */
#define SENSOR_READ_INTERVAL_MS    SAMPLE_INTERVAL_MS
#define FALL_CHECK_INTERVAL_MS     SAMPLE_INTERVAL_MS
#define VITALS_CHECK_INTERVAL_MS   1000U
#define MQTT_PUBLISH_INTERVAL_MS   5000U

/* ========== Power Management Configuration ========== */
#define POWER_MODE_DEFAULT          0      /* 0=NORMAL, 1=LOW_POWER */
#define ESPNOW_FIXED_CHANNEL        1      /* Channel for low power mode */
#define LOW_POWER_MODE_ENABLED      1      /* Compile-time feature flag */
#define WIFI_DISCONNECT_TIMEOUT_MS  30000  /* Auto-switch to low power after 30s */

/* ========== Task Priorities (higher = more important) ========== */
/* Optimized for fall detection responsiveness and power efficiency */
#define FALL_DETECT_TASK_PRIORITY  7   /* UP - Most critical (<100ms response) */
#define SENSOR_TASK_PRIORITY       6   /* UP - Feeds fall detection */
#define VITALS_TASK_PRIORITY       4   /* DOWN - Non-critical threshold checks */
#define GATEWAY_RX_TASK_PRIORITY   3   /* DOWN - Currently placeholder */
#define MQTT_TASK_PRIORITY         2   /* DOWN - Background I/O */
#define CLOUD_TX_TASK_PRIORITY     2   /* Alias for backward compatibility */

/* ========== Task Stack Sizes (bytes) ========== */
#define SENSOR_TASK_STACK_SIZE     4096U
#define FALL_TASK_STACK_SIZE       4096U
#define VITALS_TASK_STACK_SIZE     4096U
#define GATEWAY_TASK_STACK_SIZE    4096U
#define MQTT_TASK_STACK_SIZE       8192U  /* Larger for TLS */
#define CLOUD_TX_TASK_STACK_SIZE   8192U  /* Alias for backward compatibility */

#endif /* CONFIG_H */
