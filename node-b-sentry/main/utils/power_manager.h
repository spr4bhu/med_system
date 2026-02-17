/**
 * @file power_manager.h
 * @brief Power mode management for dual-mode operation (Normal/Low Power)
 *
 * This module manages power modes for the ESP32 medical monitoring system:
 * - NORMAL mode: WiFi + MQTT + ESP-NOW (full connectivity, ~160-200mA)
 * - LOW_POWER mode: ESP-NOW only on fixed channel (~40-60mA)
 *
 * Mode transitions are triggered automatically by WiFi availability:
 * - WiFi down >30s → Switch to LOW_POWER
 * - WiFi reconnects → Switch to NORMAL + upload offline logs
 *
 * @note Mode does NOT persist to NVS - always starts in NORMAL on boot
 */

#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/**
 * @brief Power mode enumeration
 */
typedef enum {
    POWER_MODE_NORMAL = 0,      /**< WiFi + MQTT + ESP-NOW (default) */
    POWER_MODE_LOW_POWER = 1    /**< ESP-NOW only, fixed channel */
} power_mode_t;

/**
 * @brief Initialize the power manager
 *
 * Sets initial mode to POWER_MODE_NORMAL (in-memory only).
 *
 * @return ESP_OK on success
 *         ESP_FAIL on initialization failure
 *
 * @note Must be called before any other power_manager functions
 */
esp_err_t power_manager_init(void);

/**
 * @brief Get the current power mode
 *
 * @return Current power mode (POWER_MODE_NORMAL or POWER_MODE_LOW_POWER)
 *
 * @note Thread-safe
 */
power_mode_t power_manager_get_mode(void);

/**
 * @brief Set the power mode and trigger transitions
 *
 * This function:
 * - Updates the current mode (in-memory)
 * - Suspends/resumes vitals task if registered
 * - Sets POWER_MODE_CHANGE_BIT event group bit
 *
 * @param mode New power mode to activate
 *
 * @return ESP_OK on success
 *         ESP_ERR_INVALID_ARG if mode is invalid
 *
 * @note Called by WiFi event handler (automatic transitions only)
 * @note Mode is NOT saved to NVS - resets to NORMAL on reboot
 */
esp_err_t power_manager_set_mode(power_mode_t mode);

/**
 * @brief Register the vitals monitoring task for suspension in low power mode
 *
 * The vitals task will be suspended when entering low power mode and
 * resumed when returning to normal mode.
 *
 * @param task_handle FreeRTOS task handle for vitals_monitor_task
 *
 * @note Call this from main.c after creating vitals_monitor_task
 */
void power_manager_register_vitals_task(TaskHandle_t task_handle);

#endif /* POWER_MANAGER_H */
