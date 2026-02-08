#include "sensor_task.h"
#include "config.h"
#include "utils/data_structures.h"
#include "sensors/mpu6050.h"
#include "sensors/hw827.h"
#include "sensors/dht22.h"
#include "sensors/emergency_button.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_log.h"

static const char *TAG = "SENSOR_TASK";

void sensor_task(void *pvParameters)
{
    (void)pvParameters;  /* MISRA Rule 2.7 */

    ESP_LOGI(TAG, "Sensor task started");

    sensor_data_t sensor_data = {0};
    hw827_data_t hw827_prev = {0};
    uint32_t hw827_peaks = 0U;
    uint32_t last_dht11_read = 0U;
    uint32_t last_hw827_read = 0U;
    bool emergency_button_was_pressed = false;

    while (1) {
        uint32_t current_time = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);

        /* Read DHT11 every 2 seconds with retry on failure */
        if ((current_time - last_dht11_read) >= DHT11_READ_INTERVAL_MS) {
            int result = -1;
            /* Retry up to 3 times on checksum/timeout */
            for (int retry = 0; retry < 3; retry++) {
                result = dht11_read(&sensor_data.temperature);
                if (result == 0) {
                    /* Success */
                    if ((sensor_data.temperature.temperature > 0.0f) ||
                        (sensor_data.temperature.humidity > 0U)) {
                        ESP_LOGI(TAG, "DHT11: Temp=%.1fC, Humidity=%d%%%s",
                                 sensor_data.temperature.temperature,
                                 sensor_data.temperature.humidity,
                                 (retry > 0) ? " (retry)" : "");
                    } else {
                        ESP_LOGW(TAG, "DHT11: Values are 0 (sensor disconnected?)");
                    }
                    break;
                } else {
                    /* Failure - wait 100ms before retry */
                    if (retry < 2) {
                        vTaskDelay(pdMS_TO_TICKS(100));
                    } else {
                        /* Final failure */
                        if (result == -1) {
                            ESP_LOGW(TAG, "DHT11: Timeout on GPIO %d (after %d retries)",
                                     DHT11_GPIO_PIN, retry + 1);
                        } else {
                            ESP_LOGW(TAG, "DHT11: Checksum error (after %d retries)",
                                     retry + 1);
                        }
                    }
                }
            }
            last_dht11_read = current_time;
        } else {
            /* Not time for DHT11 */
        }

        /* Read HW-827 every 50ms (20Hz) */
        if ((current_time - last_hw827_read) >= HW827_SAMPLE_INTERVAL) {
            hw827_data_t current_reading = {0};
            if (hw827_read(&current_reading) == ESP_OK) {
                current_reading.peak_detected = hw827_detect_peak(&current_reading, &hw827_prev);

                if (current_reading.peak_detected != 0U) {
                    hw827_peaks++;
                } else {
                    /* No peak */
                }

                /* Log every 10 samples (500ms) */
                static uint32_t hw827_sample_count = 0U;
                hw827_sample_count++;
                if ((hw827_sample_count % 10U) == 0U) {
                    ESP_LOGI(TAG, "HW-827: ADC=%04u, Voltage=%u mV, Peaks=%lu",
                             current_reading.adc_raw,
                             (unsigned int)current_reading.voltage_mv,
                             (unsigned long)hw827_peaks);
                } else {
                    /* Not time to log */
                }

                sensor_data.heart_rate = current_reading;
                hw827_prev = current_reading;
            } else {
                ESP_LOGW(TAG, "HW-827 read failed");
            }
            last_hw827_read = current_time;
        } else {
            /* Not time for HW-827 */
        }

        /* Check emergency button (polling) */
        bool button_pressed = is_emergency_button_pressed();
        if (button_pressed && !emergency_button_was_pressed) {
            ESP_LOGW(TAG, "========================================");
            ESP_LOGW(TAG, "EMERGENCY BUTTON PRESSED! (GPIO%d)", EMERGENCY_BUTTON_PIN);
            ESP_LOGW(TAG, "========================================");

            (void)xEventGroupSetBits(emergency_event_group, SOS_PRESSED_BIT);
        } else if (!button_pressed && emergency_button_was_pressed) {
            ESP_LOGI(TAG, "Emergency button released (GPIO%d)", EMERGENCY_BUTTON_PIN);
        } else {
            /* No change */
        }
        emergency_button_was_pressed = button_pressed;

        /* Update timestamp */
        sensor_data.last_update_ms = current_time;

        /* Send to queue */
        BaseType_t queue_ret = xQueueOverwrite(sensor_data_queue, &sensor_data);
        if (queue_ret != pdPASS) {
            ESP_LOGW(TAG, "Failed to update sensor data queue");
        } else {
            /* Queue updated */
        }

        vTaskDelay(pdMS_TO_TICKS(SENSOR_READ_INTERVAL_MS));
    }

    vTaskDelete(NULL);
}
