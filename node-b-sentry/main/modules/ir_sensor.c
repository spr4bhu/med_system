#include "ir_sensor.h"
#include "config.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "IR_SENSOR";

static uint32_t last_trigger_time = 0U;

esp_err_t ir_sensor_init(void) {
    ESP_LOGI(TAG, "Initializing IR sensor...");

    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << IR_SENSOR_GPIO);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;

    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "GPIO config failed: %d", ret);
        return ret;
    } else {
        ESP_LOGI(TAG, "IR sensor initialized successfully");
    }

    return ESP_OK;
}

bool ir_sensor_read(void) {
    int level = gpio_get_level(IR_SENSOR_GPIO);
    uint32_t current_time = (uint32_t)xTaskGetTickCount();

    /* Check if motion detected (assuming active LOW) */
    if (level == 0) {
        /* Check cooldown period */
        if ((current_time - last_trigger_time) >= pdMS_TO_TICKS(IR_COOLDOWN_MS)) {
            last_trigger_time = current_time;
            ESP_LOGI(TAG, "Motion detected!");
            return true;
        } else {
            /* Still in cooldown */
            ESP_LOGD(TAG, "Motion detected but in cooldown period");
            return false;
        }
    } else {
        /* No motion */
        return false;
    }
}
