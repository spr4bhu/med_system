/**
 * @file offline_logger.h
 * @brief Offline event logging system for medical monitoring data
 *
 * This module provides persistent event storage when WiFi/MQTT is unavailable:
 * - SPIFFS-based circular buffer (2 files, 64KB total capacity)
 * - Stores ~1280 events (~21 hours at 1 event/minute)
 * - Automatic upload to MQTT when WiFi reconnects
 * - NVS metadata for file index and upload cursor
 *
 * Event Priority:
 * - Priority 1 (always logged): Falls, button presses, vitals alerts, intrusions
 * - Priority 2 (if space): Sensor snapshots (60s intervals)
 */

#ifndef OFFLINE_LOGGER_H
#define OFFLINE_LOGGER_H

#include "esp_err.h"
#include <stdint.h>

/**
 * @brief Event type enumeration
 */
typedef enum {
    EVENT_TYPE_FALL = 1,         /**< Fall detection event */
    EVENT_TYPE_BUTTON = 2,       /**< Emergency button press */
    EVENT_TYPE_HIGH_TEMP = 3,    /**< High temperature alert */
    EVENT_TYPE_HIGH_HR = 4,      /**< High heart rate alert */
    EVENT_TYPE_INTRUSION = 5,    /**< Security intrusion (Node B) */
    EVENT_TYPE_SENSOR_SNAPSHOT = 6  /**< Periodic sensor snapshot */
} event_type_t;

/**
 * @brief Emergency reason bitfield (can combine multiple reasons)
 */
#define EMERGENCY_FALL       (1 << 0)  /**< Fall detected */
#define EMERGENCY_BUTTON     (1 << 1)  /**< SOS button pressed */
#define EMERGENCY_HIGH_TEMP  (1 << 2)  /**< Temperature threshold exceeded */
#define EMERGENCY_HIGH_HR    (1 << 3)  /**< Heart rate threshold exceeded */
#define EMERGENCY_INTRUSION  (1 << 4)  /**< Unauthorized access */

/**
 * @brief Offline event structure (50 bytes)
 */
typedef struct {
    uint32_t timestamp;          /**< Seconds since boot (or Unix if NTP synced) */
    uint8_t type;                /**< Event type (event_type_t) */
    char posture[16];            /**< "STANDING", "SITTING", "LYING", or "" */
    float sensor_value;          /**< Relevant sensor reading (temp, HR, jerk, etc.) */
    uint8_t emergency_reasons;   /**< Bitfield of emergency reasons */
} offline_event_t;

/**
 * @brief Initialize the offline logger
 *
 * This function:
 * - Mounts SPIFFS partition (64KB)
 * - Loads metadata from NVS (active file index, upload cursor)
 * - Creates circular buffer files if not present
 *
 * @return ESP_OK on success
 *         ESP_ERR_NOT_FOUND if SPIFFS partition not found
 *         ESP_FAIL on mount/initialization failure
 *
 * @note Must be called before any logging operations
 * @note SPIFFS must be defined in partition table
 */
esp_err_t offline_logger_init(void);

/**
 * @brief Log an event to offline storage
 *
 * Appends the event to the active log file in CSV format.
 * If the file exceeds 32KB, switches to the other file (circular buffer).
 *
 * @param event Pointer to event structure to log
 *
 * @return ESP_OK on success
 *         ESP_ERR_INVALID_ARG if event is NULL
 *         ESP_FAIL on write failure
 *
 * @note Thread-safe (uses mutex)
 * @note Oldest events are overwritten when buffer is full
 */
esp_err_t offline_logger_log_event(const offline_event_t *event);

/**
 * @brief Upload all pending offline events to MQTT
 *
 * Reads events from log files, publishes to MQTT topic "node_a/offline_events",
 * and truncates files on successful upload.
 *
 * @return ESP_OK on success (all events uploaded)
 *         ESP_ERR_INVALID_STATE if MQTT not connected
 *         ESP_FAIL on read/publish failure
 *
 * @note Blocks until all events are published
 * @note Called automatically by mqtt_task when WiFi reconnects
 */
esp_err_t offline_logger_upload_pending(void);

/**
 * @brief Get the number of pending offline events
 *
 * @return Number of events waiting to be uploaded (0 if none)
 *
 * @note Counts events across both log files
 */
uint32_t offline_logger_get_pending_count(void);

/**
 * @brief Deinitialize the offline logger and unmount SPIFFS
 *
 * @return ESP_OK on success
 *
 * @note Call during system shutdown (optional)
 */
esp_err_t offline_logger_deinit(void);

#endif /* OFFLINE_LOGGER_H */
