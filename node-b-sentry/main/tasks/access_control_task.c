#include "access_control_task.h"
#include "config.h"
#include "utils/data_structures.h"
#include "modules/rfid_rc522.h"
#include "modules/buzzer.h"
#include "message_types.h"
#include "freertos/queue.h"
#include "esp_log.h"

static const char* TAG = "ACCESS_CTRL";

void access_control_task(void* pvParameters) {
    (void)pvParameters;  /* Unused parameter (MISRA Rule 2.7) */

    ESP_LOGI(TAG, "Access control task started");

    while (1) {
        /* Scan for RFID card */
        uint32_t uid = 0U;
        esp_err_t ret = rfid_rc522_read_uid(&uid);

        if (ret == ESP_OK) {
            /* Check authorization */
            bool authorized = rfid_rc522_is_authorized(uid);

            ESP_LOGI(TAG, "RFID scanned: UID=0x%08lX, Authorized=%d",
                     (unsigned long)uid, authorized);

            /* Create access event */
            access_event_t access_event;
            access_event.rfid_uid = uid;
            access_event.access_granted = authorized ? 1U : 0U;
            access_event.timestamp_ms = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);

            /* TODO: Package into espnow_packet_t and send to queue */
            /* For now, just log */

            if (!authorized) {
                /* Sound buzzer for unauthorized access */
                (void)buzzer_alarm_on(2000U, BUZZER_UNAUTHORIZED_MS);

                /* Set unauthorized access event bit */
                (void)xEventGroupSetBits(
                    alarm_event_group,
                    UNAUTHORIZED_ACCESS_BIT
                );

                ESP_LOGW(TAG, "Unauthorized access attempt!");
            } else {
                ESP_LOGI(TAG, "Access granted");
            }
        } else {
            /* No card detected */
        }

        /* Sleep for scan interval */
        vTaskDelay(pdMS_TO_TICKS(RFID_SCAN_INTERVAL_MS));
    }

    /* Task should never return */
    vTaskDelete(NULL);
}
