#include "intrusion_detect_task.h"
#include "config.h"
#include "utils/data_structures.h"
#include "modules/ir_sensor.h"
#include "modules/buzzer.h"
#include "message_types.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_log.h"

static const char* TAG = "INTRUSION";

void intrusion_detect_task(void* pvParameters) {
    (void)pvParameters;  /* Unused parameter (MISRA Rule 2.7) */

    ESP_LOGI(TAG, "Intrusion detection task started");

    while (1) {
        /* Read IR sensor */
        bool motion_detected = ir_sensor_read();

        if (motion_detected) {
            ESP_LOGW(TAG, "***** INTRUSION DETECTED *****");

            /* Create intrusion event */
            intrusion_event_t intrusion_event;
            intrusion_event.pir_triggered = 1U;
            intrusion_event.timestamp_ms = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);

            /* TODO: Package into espnow_packet_t and send to queue */

            /* Sound alarm */
            (void)buzzer_alarm_on(2000U, BUZZER_ALARM_DURATION_MS);

            /* Set intrusion event bit */
            (void)xEventGroupSetBits(
                alarm_event_group,
                INTRUSION_BIT
            );
        } else {
            /* No motion */
        }

        /* Sleep for check interval */
        vTaskDelay(pdMS_TO_TICKS(100U));
    }

    /* Task should never return */
    vTaskDelete(NULL);
}
