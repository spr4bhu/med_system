#include "emergency_button.h"
#include "config.h"
#include "esp_log.h"
#include "driver/gpio.h"

static const char *TAG = "BUTTON";

esp_err_t emergency_button_init(void)
{
    /* External 2.2k pull-up resistor - disable internal pull-up */
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << EMERGENCY_BUTTON_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "GPIO config failed: %s", esp_err_to_name(ret));
        return ret;
    } else {
        ESP_LOGI(TAG, "Emergency button on GPIO %d (external pull-up, polling)", EMERGENCY_BUTTON_PIN);
    }

    return ESP_OK;
}

bool is_emergency_button_pressed(void)
{
    /* Active LOW: switch connects GPIO to GND, pull-up holds HIGH when released */
    return (gpio_get_level((gpio_num_t)EMERGENCY_BUTTON_PIN) == 0);
}
