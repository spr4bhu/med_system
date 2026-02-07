#include "emergency_button.h"
#include "config.h"
#include "utils/data_structures.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

static const char* TAG = "EMERG_BTN";

/* Debounce timing */
static uint32_t last_press_time = 0U;
#define DEBOUNCE_MS 50U

/* ISR Handler (MISRA Rule 8.4 - static function) */
static void IRAM_ATTR button_isr_handler(void* arg) {
    (void)arg;  /* Unused parameter (MISRA Rule 2.7) */

    uint32_t current_time = (uint32_t)xTaskGetTickCountFromISR();

    /* Debounce check */
    if ((current_time - last_press_time) > pdMS_TO_TICKS(DEBOUNCE_MS)) {
        BaseType_t higher_priority_task_woken = pdFALSE;

        /* Set SOS event bit from ISR */
        (void)xEventGroupSetBitsFromISR(
            emergency_event_group,
            SOS_PRESSED_BIT,
            &higher_priority_task_woken
        );

        last_press_time = current_time;

        /* Yield if higher priority task woken */
        if (higher_priority_task_woken == pdTRUE) {
            portYIELD_FROM_ISR();
        } else {
            /* No yield needed (MISRA Rule 14.4 - final else) */
        }
    } else {
        /* Debounce period - ignore */
    }
}

esp_err_t emergency_button_init(void) {
    ESP_LOGI(TAG, "Initializing emergency button...");

    /* Configure GPIO as input with pull-up */
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_NEGEDGE;  /* Trigger on falling edge */
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << SOS_BUTTON_GPIO);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;

    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "GPIO config failed: %d", ret);
        return ret;
    } else {
        /* GPIO configured */
    }

    /* Install ISR service */
    ret = gpio_install_isr_service(0);
    if ((ret != ESP_OK) && (ret != ESP_ERR_INVALID_STATE)) {
        /* ESP_ERR_INVALID_STATE means already installed */
        ESP_LOGE(TAG, "ISR service install failed: %d", ret);
        return ret;
    } else {
        /* ISR service ready */
    }

    /* Attach ISR handler */
    ret = gpio_isr_handler_add(SOS_BUTTON_GPIO, button_isr_handler, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ISR handler add failed: %d", ret);
        return ret;
    } else {
        ESP_LOGI(TAG, "Emergency button initialized successfully");
    }

    return ESP_OK;
}
