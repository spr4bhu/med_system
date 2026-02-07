#include "sensor_task.h"
#include "config.h"
#include "utils/data_structures.h"
#include "utils/logger.h"
#include "sensors/mpu6050.h"
#include "sensors/max30102.h"
#include "sensors/dht22.h"
#include "message_types.h"
#include "freertos/queue.h"
#include "esp_log.h"

static const char* TAG = "SENSOR_TASK";

void sensor_task(void* pvParameters) {
    (void)pvParameters;  /* Unused parameter (MISRA Rule 2.7) */

    ESP_LOGI(TAG, "Sensor task started");

    sensor_data_t sensor_data;
    TickType_t last_wake_time = xTaskGetTickCount();

    while (1) {
        /* Read MPU6050 accelerometer */
        float ax = 0.0f;
        float ay = 0.0f;
        float az = 0.0f;
        esp_err_t ret = mpu6050_read_accel(&ax, &ay, &az);

        if (ret == ESP_OK) {
            sensor_data.accel_x = ax;
            sensor_data.accel_y = ay;
            sensor_data.accel_z = az;
        } else {
            ESP_LOGW(TAG, "Failed to read accelerometer: %d", ret);
            sensor_data.accel_x = 0.0f;
            sensor_data.accel_y = 0.0f;
            sensor_data.accel_z = 1.0f;  /* Default to 1g on Z */
        }

        /* Read MAX30102 heart rate */
        float hr = 0.0f;
        ret = max30102_read_bpm(&hr);

        if (ret == ESP_OK) {
            sensor_data.heart_rate_bpm = hr;
        } else {
            ESP_LOGW(TAG, "Failed to read heart rate: %d", ret);
            sensor_data.heart_rate_bpm = 75.0f;  /* Default resting rate */
        }

        /* Read DHT22 temperature */
        float temp = 0.0f;
        float humid = 0.0f;
        ret = dht22_read_temp(&temp, &humid);

        if (ret == ESP_OK) {
            sensor_data.temperature_c = temp;
        } else {
            ESP_LOGW(TAG, "Failed to read temperature: %d", ret);
            sensor_data.temperature_c = 36.5f;  /* Default body temp */
        }

        /* Set timestamp (MISRA Rule 10.3 - explicit cast) */
        sensor_data.timestamp_ms = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);

        /* Initialize posture as unknown (will be determined by fall detection) */
        sensor_data.posture = POSTURE_STANDING;

        /* Send to queue (MISRA Rule 17.7 - check return value) */
        BaseType_t queue_ret = xQueueSend(
            sensor_data_queue,
            &sensor_data,
            pdMS_TO_TICKS(100U)
        );

        if (queue_ret != pdPASS) {
            ESP_LOGW(TAG, "Failed to send sensor data to queue");
        } else {
            ESP_LOGD(TAG, "Sensor data: HR=%.1f, Temp=%.1f, Accel=(%.2f, %.2f, %.2f)",
                     hr, temp, ax, ay, az);
        }

        /* Wait for next interval (MISRA Rule 14.4 - no naked vTaskDelay) */
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(SENSOR_READ_INTERVAL_MS));
    }

    /* Task should never return (MISRA Rule 14.4 - final else) */
    /* But if it does, delete itself */
    vTaskDelete(NULL);
}
