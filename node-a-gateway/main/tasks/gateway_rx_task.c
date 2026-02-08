#include "gateway_rx_task.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "GATEWAY_RX";

void gateway_rx_task(void *pvParameters)
{
    (void)pvParameters;  /* MISRA Rule 2.7 */

    ESP_LOGI(TAG, "Gateway RX task started");
    ESP_LOGI(TAG, "Note: ESP-NOW receive callback is registered in mqtt_task");

    /* This task is now a placeholder - ESP-NOW RX is handled via callback in mqtt_task */
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(5000U));
    }

    vTaskDelete(NULL);
}
