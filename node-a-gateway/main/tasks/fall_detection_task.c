#include "fall_detection_task.h"
#include "config.h"
#include "utils/data_structures.h"
#include "message_types.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include <math.h>

static const char* TAG = "FALL_DETECT";

/* Fall detection state machine */
typedef enum {
    FALL_STATE_IDLE = 0,
    FALL_STATE_FREEFALL = 1,
    FALL_STATE_IMPACT = 2,
    FALL_STATE_CONFIRMED = 3
} fall_state_t;

void fall_detection_task(void* pvParameters) {
    (void)pvParameters;  /* Unused parameter (MISRA Rule 2.7) */

    ESP_LOGI(TAG, "Fall detection task started");

    fall_state_t state = FALL_STATE_IDLE;
    uint32_t freefall_start_time = 0U;
    uint32_t last_fall_time = 0U;
    sensor_data_t sensor_data;

    while (1) {
        /* Receive sensor data from queue (peek, don't remove) */
        BaseType_t ret = xQueuePeek(
            sensor_data_queue,
            &sensor_data,
            pdMS_TO_TICKS(FALL_CHECK_INTERVAL_MS)
        );

        if (ret == pdPASS) {
            /* Calculate acceleration magnitude */
            float magnitude = sqrtf(
                (sensor_data.accel_x * sensor_data.accel_x) +
                (sensor_data.accel_y * sensor_data.accel_y) +
                (sensor_data.accel_z * sensor_data.accel_z)
            );

            uint32_t current_time = (uint32_t)xTaskGetTickCount();

            /* Check debounce period (MISRA Rule 14.4 - final else) */
            if ((current_time - last_fall_time) < pdMS_TO_TICKS(FALL_DEBOUNCE_MS)) {
                /* Still in cooldown period */
                state = FALL_STATE_IDLE;
            } else {
                /* Cooldown expired - process normally */

                /* State machine logic */
                if (state == FALL_STATE_IDLE) {
                    /* Check for free-fall condition */
                    if (magnitude < FREEFALL_THRESHOLD_G) {
                        state = FALL_STATE_FREEFALL;
                        freefall_start_time = current_time;
                        ESP_LOGI(TAG, "Free-fall detected: mag=%.2f", magnitude);
                    } else {
                        /* No free-fall */
                    }
                } else if (state == FALL_STATE_FREEFALL) {
                    uint32_t freefall_duration = current_time - freefall_start_time;

                    /* Check if still in free-fall */
                    if (magnitude < FREEFALL_THRESHOLD_G) {
                        /* Continue free-fall */
                        if (freefall_duration > pdMS_TO_TICKS(50U)) {
                            /* Transition to impact detection */
                            state = FALL_STATE_IMPACT;
                            ESP_LOGI(TAG, "Free-fall confirmed, waiting for impact");
                        } else {
                            /* Keep waiting */
                        }
                    } else if (magnitude > FALL_THRESHOLD_G) {
                        /* Impact detected during free-fall */
                        state = FALL_STATE_CONFIRMED;
                        ESP_LOGI(TAG, "Impact detected: mag=%.2f", magnitude);
                    } else {
                        /* False alarm - return to idle */
                        state = FALL_STATE_IDLE;
                    }
                } else if (state == FALL_STATE_IMPACT) {
                    /* Waiting for impact */
                    uint32_t elapsed = current_time - freefall_start_time;

                    if (magnitude > FALL_THRESHOLD_G) {
                        /* Impact confirmed */
                        state = FALL_STATE_CONFIRMED;
                        ESP_LOGI(TAG, "Impact confirmed: mag=%.2f", magnitude);
                    } else if (elapsed > pdMS_TO_TICKS(500U)) {
                        /* Timeout - no impact detected */
                        state = FALL_STATE_IDLE;
                        ESP_LOGW(TAG, "Impact timeout - false alarm");
                    } else {
                        /* Keep waiting for impact */
                    }
                } else if (state == FALL_STATE_CONFIRMED) {
                    /* Fall confirmed - set event bit */
                    (void)xEventGroupSetBits(
                        emergency_event_group,
                        FALL_DETECTED_BIT
                    );

                    ESP_LOGE(TAG, "***** FALL DETECTED *****");

                    /* Start cooldown period */
                    last_fall_time = current_time;
                    state = FALL_STATE_IDLE;
                } else {
                    /* Invalid state - reset (MISRA Rule 14.4 - final else) */
                    state = FALL_STATE_IDLE;
                }
            }
        } else {
            /* Queue empty or timeout */
            ESP_LOGD(TAG, "No sensor data available");
        }

        /* Sleep for fall check interval */
        vTaskDelay(pdMS_TO_TICKS(FALL_CHECK_INTERVAL_MS));
    }

    /* Task should never return */
    vTaskDelete(NULL);
}
