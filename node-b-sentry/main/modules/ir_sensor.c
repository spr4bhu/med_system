#include "ir_sensor.h"
#include "config.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "IR_SENSOR";

esp_err_t ir_sensor_init(void)
{
    ESP_LOGI(TAG, "Initializing IR sensor on GPIO %d...", IR_SENSOR_PIN);

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << IR_SENSOR_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "GPIO config failed: %s", esp_err_to_name(ret));
        return ret;
    } else {
        ESP_LOGI(TAG, "IR sensor initialized successfully");
    }

    return ESP_OK;
}

bool ir_sensor_is_detected(void)
{
    /* IR sensor outputs LOW when object is detected */
    return (gpio_get_level(IR_SENSOR_PIN) == 0);
}
