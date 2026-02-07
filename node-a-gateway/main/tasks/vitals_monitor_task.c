#include "vitals_monitor_task.h"
#include "config.h"
#include "utils/data_structures.h"
#include "message_types.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_log.h"

static const char* TAG = "VITALS_MON";

void vitals_monitor_task(void* pvParameters) {
    (void)pvParameters;  /* Unused parameter (MISRA Rule 2.7) */

    ESP_LOGI(TAG, "Vitals monitoring task started");

    sensor_data_t sensor_data;

    while (1) {
        /* Receive sensor data from queue */
        BaseType_t ret = xQueuePeek(
            sensor_data_queue,
            &sensor_data,
            pdMS_TO_TICKS(VITALS_CHECK_INTERVAL_MS)
        );

        if (ret == pdPASS) {
            bool abnormal = false;

            /* Check heart rate thresholds (MISRA Rule 14.4 - final else) */
            if (sensor_data.heart_rate_bpm > MAX_HEART_RATE_BPM) {
                ESP_LOGW(TAG, "Heart rate too high: %.1f BPM", sensor_data.heart_rate_bpm);
                abnormal = true;
            } else if (sensor_data.heart_rate_bpm < MIN_HEART_RATE_BPM) {
                ESP_LOGW(TAG, "Heart rate too low: %.1f BPM", sensor_data.heart_rate_bpm);
                abnormal = true;
            } else {
                /* Heart rate normal */
            }

            /* Check temperature thresholds (MISRA Rule 14.4 - final else) */
            if (sensor_data.temperature_c > MAX_TEMP_C) {
                ESP_LOGW(TAG, "Temperature too high: %.1f °C", sensor_data.temperature_c);
                abnormal = true;
            } else if (sensor_data.temperature_c < MIN_TEMP_C) {
                ESP_LOGW(TAG, "Temperature too low: %.1f °C", sensor_data.temperature_c);
                abnormal = true;
            } else {
                /* Temperature normal */
            }

            /* Set event bit if vitals abnormal */
            if (abnormal) {
                (void)xEventGroupSetBits(
                    emergency_event_group,
                    VITALS_ABNORMAL_BIT
                );
                ESP_LOGE(TAG, "Abnormal vitals detected!");
            } else {
                /* Clear abnormal bit */
                (void)xEventGroupClearBits(
                    emergency_event_group,
                    VITALS_ABNORMAL_BIT
                );
            }

            ESP_LOGD(TAG, "Vitals check: HR=%.1f, Temp=%.1f",
                     sensor_data.heart_rate_bpm,
                     sensor_data.temperature_c);
        } else {
            ESP_LOGW(TAG, "No sensor data available for vitals check");
        }

        /* Sleep for check interval */
        vTaskDelay(pdMS_TO_TICKS(VITALS_CHECK_INTERVAL_MS));
    }

    /* Task should never return */
    vTaskDelete(NULL);
}
