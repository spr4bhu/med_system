#include "hw827.h"
#include "config.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "HW827";

/* Pulse counting state */
static volatile uint32_t pulse_count = 0U;
static uint32_t last_bpm_calc_time = 0U;
static float last_bpm = 75.0f;  /* Default resting heart rate */

/* ISR Handler for pulse detection (MISRA Rule 8.4 - static function) */
static void IRAM_ATTR hw827_pulse_isr_handler(void* arg) {
    (void)arg;  /* Unused parameter (MISRA Rule 2.7) */
    pulse_count++;
}

esp_err_t hw827_init(void) {
    ESP_LOGI(TAG, "Initializing HW827 heart rate sensor...");

    /* Configure GPIO as input with pull-up */
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_POSEDGE;  /* Trigger on rising edge (pulse detected) */
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << HW827_PULSE_GPIO);
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
    ret = gpio_isr_handler_add(HW827_PULSE_GPIO, hw827_pulse_isr_handler, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ISR handler add failed: %d", ret);
        return ret;
    } else {
        ESP_LOGI(TAG, "HW827 initialized successfully");
    }

    last_bpm_calc_time = (uint32_t)xTaskGetTickCount();
    return ESP_OK;
}

esp_err_t hw827_read_bpm(float* heart_rate) {
    if (heart_rate == NULL) {
        return ESP_ERR_INVALID_ARG;
    } else {
        /* Valid pointer */
    }

    uint32_t current_time = (uint32_t)xTaskGetTickCount();
    uint32_t elapsed_ms = (current_time - last_bpm_calc_time) * portTICK_PERIOD_MS;

    /* Calculate BPM every second */
    if (elapsed_ms >= 1000U) {
        if (pulse_count > 0U) {
            /* Calculate BPM: (pulses / seconds) * 60 */
            float seconds = (float)elapsed_ms / 1000.0f;
            float bpm = ((float)pulse_count / seconds) * 60.0f;

            /* Clamp to reasonable range */
            if (bpm > MAX_HEART_RATE_BPM) {
                last_bpm = MAX_HEART_RATE_BPM;
            } else if (bpm < MIN_HEART_RATE_BPM) {
                last_bpm = MIN_HEART_RATE_BPM;
            } else {
                last_bpm = bpm;
            }

            ESP_LOGD(TAG, "Pulses: %lu, BPM: %.1f", (unsigned long)pulse_count, last_bpm);
        } else {
            /* No pulses detected - sensor might not be in contact */
            ESP_LOGD(TAG, "No pulses detected");
        }

        /* Reset counters */
        pulse_count = 0U;
        last_bpm_calc_time = current_time;
    } else {
        /* Not enough time elapsed */
    }

    *heart_rate = last_bpm;
    return ESP_OK;
}
