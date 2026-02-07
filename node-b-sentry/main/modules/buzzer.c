#include "buzzer.h"
#include "config.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "BUZZER";

#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL            LEDC_CHANNEL_0
#define LEDC_DUTY_RES           LEDC_TIMER_8_BIT  /* 8-bit resolution */
#define LEDC_DUTY               128  /* 50% duty cycle */

static TaskHandle_t buzzer_timer_task = NULL;

/* Task to auto-turn off buzzer after duration */
static void buzzer_timer(void* pvParameters) {
    uint32_t duration_ms = *(uint32_t*)pvParameters;

    vTaskDelay(pdMS_TO_TICKS(duration_ms));

    /* Turn off buzzer */
    (void)ledc_stop(LEDC_MODE, LEDC_CHANNEL, 0U);
    ESP_LOGI(TAG, "Buzzer auto-off after %lu ms", (unsigned long)duration_ms);

    /* Delete self */
    buzzer_timer_task = NULL;
    vTaskDelete(NULL);
}

esp_err_t buzzer_init(void) {
    ESP_LOGI(TAG, "Initializing buzzer...");

    /* Configure LEDC timer */
    ledc_timer_config_t ledc_timer;
    ledc_timer.speed_mode       = LEDC_MODE;
    ledc_timer.duty_resolution  = LEDC_DUTY_RES;
    ledc_timer.timer_num        = LEDC_TIMER;
    ledc_timer.freq_hz          = 2000U;  /* Default frequency */
    ledc_timer.clk_cfg          = LEDC_AUTO_CLK;

    esp_err_t ret = ledc_timer_config(&ledc_timer);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LEDC timer config failed: %d", ret);
        return ret;
    } else {
        /* Timer configured */
    }

    /* Configure LEDC channel */
    ledc_channel_config_t ledc_channel;
    ledc_channel.gpio_num   = BUZZER_GPIO;
    ledc_channel.speed_mode = LEDC_MODE;
    ledc_channel.channel    = LEDC_CHANNEL;
    ledc_channel.intr_type  = LEDC_INTR_DISABLE;
    ledc_channel.timer_sel  = LEDC_TIMER;
    ledc_channel.duty       = 0U;  /* Start with buzzer off */
    ledc_channel.hpoint     = 0;

    ret = ledc_channel_config(&ledc_channel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LEDC channel config failed: %d", ret);
        return ret;
    } else {
        ESP_LOGI(TAG, "Buzzer initialized successfully");
    }

    return ESP_OK;
}

esp_err_t buzzer_alarm_on(uint32_t frequency, uint32_t duration_ms) {
    ESP_LOGI(TAG, "Buzzer alarm ON: %lu Hz for %lu ms",
             (unsigned long)frequency, (unsigned long)duration_ms);

    /* Set frequency */
    esp_err_t ret = ledc_set_freq(LEDC_MODE, LEDC_TIMER, frequency);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set frequency: %d", ret);
        return ret;
    } else {
        /* Frequency set */
    }

    /* Set duty cycle (50%) */
    ret = ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, LEDC_DUTY);
    if (ret != ESP_OK) {
        return ret;
    } else {
        /* Duty set */
    }

    /* Update duty */
    ret = ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
    if (ret != ESP_OK) {
        return ret;
    } else {
        /* Updated */
    }

    /* Cancel existing timer if any */
    if (buzzer_timer_task != NULL) {
        vTaskDelete(buzzer_timer_task);
        buzzer_timer_task = NULL;
    } else {
        /* No existing timer */
    }

    /* Create auto-off timer task */
    static uint32_t duration_param;
    duration_param = duration_ms;

    BaseType_t task_ret = xTaskCreate(
        buzzer_timer,
        "buzzer_timer",
        2048U,
        &duration_param,
        1,
        &buzzer_timer_task
    );

    if (task_ret != pdPASS) {
        ESP_LOGW(TAG, "Failed to create buzzer timer task");
    } else {
        /* Timer task created */
    }

    return ESP_OK;
}

esp_err_t buzzer_alarm_off(void) {
    ESP_LOGI(TAG, "Buzzer alarm OFF");

    /* Stop buzzer */
    esp_err_t ret = ledc_stop(LEDC_MODE, LEDC_CHANNEL, 0U);

    /* Cancel timer task if running */
    if (buzzer_timer_task != NULL) {
        vTaskDelete(buzzer_timer_task);
        buzzer_timer_task = NULL;
    } else {
        /* No timer to cancel */
    }

    return ret;
}
