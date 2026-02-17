/* Configuration file - Replace placeholder values with your credentials */

#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>
#include <stdbool.h>

/* ========== WiFi Configuration ========== */
#define WIFI_SSID       "YOUR_WIFI_SSID"
#define WIFI_PASS       "YOUR_WIFI_PASSWORD"

/* ========== MQTT Configuration (HiveMQ Cloud) ========== */
#define MQTT_BROKER_URI "mqtts://YOUR_BROKER.s1.eu.hivemq.cloud:8883"
#define MQTT_USERNAME   "YOUR_MQTT_USERNAME"
#define MQTT_PASSWORD   "YOUR_MQTT_PASSWORD"
#define USE_HIVEMQ_CLOUD 1   /* Set to 0 for local MQTT */

/* ========== ESP-NOW Configuration ========== */
#define ESP_NOW_CHANNEL  1

/* ========== GPIO Pin Configuration - Node B ========== */
#define IR_SENSOR_PIN    13
#define BUZZER_PIN       5

/* ========== RC522 RFID Pin Configuration ========== */
#define RC522_MISO_PIN    25
#define RC522_MOSI_PIN    23
#define RC522_SCLK_PIN    19
#define RC522_CS_PIN      22
#define RC522_RST_PIN     (-1)  /* Using soft-reset */

/* ========== RC522 Registers ========== */
#define RC522_REG_COMMAND         0x01U
#define RC522_REG_COM_IRQ         0x04U
#define RC522_REG_DIV_IRQ         0x05U
#define RC522_REG_ERROR           0x06U
#define RC522_REG_STATUS2         0x08U
#define RC522_REG_FIFO_DATA       0x09U
#define RC522_REG_FIFO_LEVEL      0x0AU
#define RC522_REG_CONTROL         0x0CU
#define RC522_REG_BIT_FRAMING     0x0DU
#define RC522_REG_MODE            0x11U
#define RC522_REG_TX_CONTROL      0x14U
#define RC522_REG_TX_ASK          0x15U
#define RC522_REG_CRC_RESULT_L    0x21U
#define RC522_REG_CRC_RESULT_H    0x22U
#define RC522_REG_MOD_WIDTH       0x24U
#define RC522_REG_TIMER_MODE      0x2AU
#define RC522_REG_TIMER_PRESCALER 0x2BU
#define RC522_REG_TIMER_RELOAD_H  0x2CU
#define RC522_REG_TIMER_RELOAD_L  0x2DU
#define RC522_REG_VERSION         0x37U

/* ========== RC522 Commands ========== */
#define RC522_CMD_IDLE          0x00U
#define RC522_CMD_MEM           0x01U
#define RC522_CMD_CALC_CRC      0x03U
#define RC522_CMD_TRANSMIT      0x04U
#define RC522_CMD_RECEIVE       0x08U
#define RC522_CMD_TRANSCEIVE    0x0CU
#define RC522_CMD_MF_AUTHENT    0x0EU
#define RC522_CMD_SOFT_RESET    0x0FU

/* ========== PICC Commands ========== */
#define PICC_CMD_REQA           0x26U
#define PICC_CMD_SEL_CL1        0x93U
#define PICC_CMD_SEL_CL2        0x95U

/* ========== RC522 Status Codes ========== */
#define RC522_STATUS_OK         0U
#define RC522_STATUS_ERROR      1U
#define RC522_STATUS_COLLISION  2U
#define RC522_STATUS_TIMEOUT    3U
#define RC522_STATUS_NO_ROOM    4U
#define RC522_STATUS_CRC_WRONG  5U

/* ========== Security System States ========== */
typedef enum {
    STATE_IDLE        = 0,   /* No IR detection */
    STATE_IR_DETECTED = 1,   /* IR active, waiting for RFID or timeout */
    STATE_AUTHORIZED  = 2,   /* RFID verified during IR detection */
    STATE_INTRUSION   = 3    /* IR active but no RFID after timeout */
} security_state_t;

/* ========== Timing Constants ========== */
#define IR_WINDOW_MS        5000U   /* 5-second window for RFID verification */
#define RFID_POLL_MS        100U    /* Poll RFID every 100ms */
#define IR_CHECK_MS         50U     /* Check IR sensor every 50ms */

/* ========== RFID Database ========== */
typedef struct {
    uint8_t uid[4];
    const char *name;
    bool authorized;
} rfid_entry_t;

/* ========== ESP-NOW Emergency Message (must match Node A) ========== */
typedef struct {
    uint8_t msg_type;           /* 1 = Emergency alert, 2 = Clear */
    uint8_t emergency_reasons;  /* Bitfield of emergency reasons */
    char posture[16];           /* Current posture string */
    float sensor_value;         /* Relevant sensor value */
} espnow_emergency_msg_t;

/* ========== Power Management Configuration ========== */
#define POWER_MODE_DEFAULT          0      /* 0=NORMAL, 1=LOW_POWER */
#define ESPNOW_FIXED_CHANNEL        1      /* Channel for low power mode */
#define LOW_POWER_MODE_ENABLED      1      /* Compile-time feature flag */
#define WIFI_DISCONNECT_TIMEOUT_MS  30000  /* Auto-switch to low power after 30s */

/* ========== Task Priorities ========== */
/* Optimized for security response and power efficiency */
#define SECURITY_TASK_PRIORITY     7   /* UP - Critical security response */
#define MQTT_TASK_PRIORITY         3   /* DOWN - Background I/O */
#define ESPNOW_RX_TASK_PRIORITY    4   /* Unchanged - Moderate priority */

/* ========== Task Stack Sizes (bytes) ========== */
#define SECURITY_TASK_STACK_SIZE   4096U
#define MQTT_TASK_STACK_SIZE       8192U
#define ESPNOW_RX_TASK_STACK_SIZE  4096U

/* ========== MQTT Publish Interval ========== */
#define MQTT_PUBLISH_INTERVAL_MS   5000U

#endif /* CONFIG_H */
