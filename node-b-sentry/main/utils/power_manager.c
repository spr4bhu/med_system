/**
 * @file power_manager.c
 * @brief Power mode management implementation
 */

#include "power_manager.h"
#include "data_structures.h"
#include "esp_log.h"
#include "freertos/event_groups.h"
#include <string.h>

static const char *TAG = "power_manager";

/**
 * @brief Internal state structure
 */
typedef struct {
    power_mode_t current_mode;           /**< Current power mode (in-memory) */
    bool transition_pending;             /**< Flag for mode transition in progress */
    TaskHandle_t vitals_task_handle;     /**< Handle for vitals task suspension */
} power_manager_state_t;

static power_manager_state_t s_state = {
    .current_mode = POWER_MODE_NORMAL,
    .transition_pending = false,
    .vitals_task_handle = NULL
};

/* External event group from data_structures.c */
extern EventGroupHandle_t alarm_event_group;

esp_err_t power_manager_init(void)
{
    /* Initialize to NORMAL mode (always starts in NORMAL on boot) */
    s_state.current_mode = POWER_MODE_NORMAL;
    s_state.transition_pending = false;
    s_state.vitals_task_handle = NULL;

    ESP_LOGI(TAG, "Power manager initialized - mode: NORMAL");

    return ESP_OK;
}

power_mode_t power_manager_get_mode(void)
{
    /* Thread-safe read (atomic for enum) */
    return s_state.current_mode;
}

esp_err_t power_manager_set_mode(power_mode_t mode)
{
    esp_err_t result = ESP_OK;

    /* Validate input */
    if ((mode != POWER_MODE_NORMAL) && (mode != POWER_MODE_LOW_POWER)) {
        ESP_LOGE(TAG, "Invalid power mode: %d", mode);
        result = ESP_ERR_INVALID_ARG;
    } else {
        /* Check if mode actually changed */
        if (s_state.current_mode != mode) {
            const char *mode_str = (mode == POWER_MODE_NORMAL) ? "NORMAL" : "LOW_POWER";
            ESP_LOGI(TAG, "Power mode transition: %s -> %s",
                     (s_state.current_mode == POWER_MODE_NORMAL) ? "NORMAL" : "LOW_POWER",
                     mode_str);

            s_state.transition_pending = true;
            s_state.current_mode = mode;

            /* Suspend/resume vitals task based on mode */
            if ((mode == POWER_MODE_LOW_POWER) && (s_state.vitals_task_handle != NULL)) {
                vTaskSuspend(s_state.vitals_task_handle);
                ESP_LOGI(TAG, "Suspended vitals_monitor_task (low power mode)");
            } else if ((mode == POWER_MODE_NORMAL) && (s_state.vitals_task_handle != NULL)) {
                vTaskResume(s_state.vitals_task_handle);
                ESP_LOGI(TAG, "Resumed vitals_monitor_task (normal mode)");
            } else {
                /* Task handle not registered yet - skip suspension */
            }

            /* Notify mqtt_task of mode change via event group */
            EventBits_t bits = xEventGroupSetBits(alarm_event_group, POWER_MODE_CHANGE_BIT);
            if ((bits & POWER_MODE_CHANGE_BIT) == 0) {
                ESP_LOGE(TAG, "Failed to set POWER_MODE_CHANGE_BIT");
                result = ESP_FAIL;
            } else {
                s_state.transition_pending = false;
            }
        } else {
            ESP_LOGD(TAG, "Mode already set to %s, no transition needed",
                     (mode == POWER_MODE_NORMAL) ? "NORMAL" : "LOW_POWER");
        }
    }

    return result;
}

void power_manager_register_vitals_task(TaskHandle_t task_handle)
{
    if (task_handle != NULL) {
        s_state.vitals_task_handle = task_handle;
        ESP_LOGI(TAG, "Registered vitals task for power management");
    } else {
        ESP_LOGW(TAG, "Cannot register NULL task handle");
    }
}
