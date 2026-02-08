#include "vitals_monitor_task.h"
#include "config.h"
#include "utils/data_structures.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_log.h"

static const char *TAG = "VITALS_MON";

void vitals_monitor_task(void *pvParameters)
{
    (void)pvParameters;  /* MISRA Rule 2.7 */

    ESP_LOGI(TAG, "Vitals monitoring task started");

    sensor_data_t sensor_data;

    while (1) {
        /* Peek at latest sensor data */
        BaseType_t ret = xQueuePeek(
            sensor_data_queue,
            &sensor_data,
            pdMS_TO_TICKS(VITALS_CHECK_INTERVAL_MS)
        );

        if (ret == pdPASS) {
            bool abnormal = false;

            /* Check temperature alert (server threshold: >= 30C) */
            if (sensor_data.temperature.temperature >= TEMP_ALERT_THRESHOLD) {
                ESP_LOGW(TAG, "========================================");
                ESP_LOGW(TAG, "     HIGH TEMPERATURE ALERT!");
                ESP_LOGW(TAG, "========================================");
                ESP_LOGW(TAG, "Current: %.1fC, Threshold: %.1fC",
                         sensor_data.temperature.temperature, TEMP_ALERT_THRESHOLD);
                ESP_LOGW(TAG, "========================================");
                abnormal = true;
            } else {
                /* Temperature normal */
            }

            /* Check heart rate alert (server threshold: peak count > 120) */
            if (sensor_data.heart_rate.peak_count > (uint32_t)HR_ALERT_THRESHOLD) {
                ESP_LOGW(TAG, "========================================");
                ESP_LOGW(TAG, "     HIGH HEART RATE ALERT!");
                ESP_LOGW(TAG, "========================================");
                ESP_LOGW(TAG, "Peaks: %lu, Threshold: %d",
                         (unsigned long)sensor_data.heart_rate.peak_count, HR_ALERT_THRESHOLD);
                ESP_LOGW(TAG, "========================================");
                abnormal = true;
            } else {
                /* Heart rate normal */
            }

            if (abnormal) {
                (void)xEventGroupSetBits(emergency_event_group, VITALS_ABNORMAL_BIT);
                ESP_LOGE(TAG, "Abnormal vitals detected!");
            } else {
                (void)xEventGroupClearBits(emergency_event_group, VITALS_ABNORMAL_BIT);
            }

            ESP_LOGD(TAG, "Vitals: Temp=%.1fC, HR_voltage=%.0f mV",
                     sensor_data.temperature.temperature,
                     sensor_data.heart_rate.voltage_mv);
        } else {
            ESP_LOGW(TAG, "No sensor data available for vitals check");
        }

        vTaskDelay(pdMS_TO_TICKS(VITALS_CHECK_INTERVAL_MS));
    }

    vTaskDelete(NULL);
}
