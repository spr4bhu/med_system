#include "buzzer.h"
#include "config.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "BUZZER";

esp_err_t buzzer_init(void)
{
    ESP_LOGI(TAG, "Initializing buzzer on GPIO %d...", BUZZER_PIN);

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BUZZER_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "GPIO config failed: %s", esp_err_to_name(ret));
        return ret;
    } else {
        /* GPIO configured */
    }

    /* Start with buzzer off */
    (void)gpio_set_level(BUZZER_PIN, 0);
    ESP_LOGI(TAG, "Buzzer initialized successfully");

    return ESP_OK;
}

void buzzer_on(void)
{
    (void)gpio_set_level(BUZZER_PIN, 1);
    ESP_LOGW(TAG, "Buzzer ON");
}

void buzzer_off(void)
{
    (void)gpio_set_level(BUZZER_PIN, 0);
    ESP_LOGI(TAG, "Buzzer OFF");
}
