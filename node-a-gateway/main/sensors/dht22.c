#include "dht22.h"
#include "config.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"

static const char* TAG = "DHT22";

/* DHT22 Timing (microseconds) */
#define DHT22_START_SIGNAL_US   1000U
#define DHT22_RESPONSE_WAIT_US  40U
#define DHT22_BIT_TIMEOUT_US    100U

/* Helper function to wait for GPIO level with timeout */
static esp_err_t wait_for_level(uint32_t level, uint32_t timeout_us) {
    uint32_t start = (uint32_t)esp_timer_get_time();
    uint32_t elapsed = 0U;

    while (elapsed < timeout_us) {
        if (gpio_get_level(DHT22_GPIO) == (int)level) {
            return ESP_OK;
        } else {
            /* Continue waiting */
        }
        elapsed = (uint32_t)esp_timer_get_time() - start;
    }

    return ESP_ERR_TIMEOUT;
}

esp_err_t dht22_init(void) {
    ESP_LOGI(TAG, "Initializing DHT22...");

    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << DHT22_GPIO);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;

    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "GPIO config failed: %d", ret);
        return ret;
    } else {
        /* Set initial state high */
        (void)gpio_set_level(DHT22_GPIO, 1);
        ESP_LOGI(TAG, "DHT22 initialized successfully");
    }

    return ESP_OK;
}

esp_err_t dht22_read_temp(float* temperature, float* humidity) {
    if ((temperature == NULL) || (humidity == NULL)) {
        return ESP_ERR_INVALID_ARG;
    } else {
        /* Valid pointers */
    }

    uint8_t data[5] = {0U, 0U, 0U, 0U, 0U};
    esp_err_t ret = ESP_OK;

    /* Send start signal */
    (void)gpio_set_direction(DHT22_GPIO, GPIO_MODE_OUTPUT);
    (void)gpio_set_level(DHT22_GPIO, 0);
    ets_delay_us(DHT22_START_SIGNAL_US);
    (void)gpio_set_level(DHT22_GPIO, 1);

    /* Switch to input mode */
    (void)gpio_set_direction(DHT22_GPIO, GPIO_MODE_INPUT);

    /* Wait for DHT22 response */
    ret = wait_for_level(0U, 80U);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "No response from DHT22");
        return ret;
    } else {
        /* Response received */
    }

    ret = wait_for_level(1U, 80U);
    if (ret != ESP_OK) {
        return ret;
    } else {
        /* Continue */
    }

    ret = wait_for_level(0U, 80U);
    if (ret != ESP_OK) {
        return ret;
    } else {
        /* Start reading data */
    }

    /* Read 40 bits (5 bytes) */
    for (uint8_t byte_idx = 0U; byte_idx < 5U; byte_idx++) {
        for (uint8_t bit_idx = 0U; bit_idx < 8U; bit_idx++) {
            /* Wait for bit start (low) */
            ret = wait_for_level(0U, DHT22_BIT_TIMEOUT_US);
            if (ret != ESP_OK) {
                ESP_LOGW(TAG, "Bit read timeout");
                return ret;
            } else {
                /* Continue */
            }

            /* Wait for high level */
            ret = wait_for_level(1U, DHT22_BIT_TIMEOUT_US);
            if (ret != ESP_OK) {
                return ret;
            } else {
                /* Continue */
            }

            /* Measure high pulse width */
            ets_delay_us(DHT22_RESPONSE_WAIT_US);

            if (gpio_get_level(DHT22_GPIO) == 1) {
                /* Long pulse = 1 */
                data[byte_idx] |= (uint8_t)(1U << (7U - bit_idx));
            } else {
                /* Short pulse = 0 (already 0) */
            }
        }
    }

    /* Verify checksum */
    uint8_t checksum = data[0] + data[1] + data[2] + data[3];
    if (checksum != data[4]) {
        ESP_LOGW(TAG, "Checksum mismatch: expected 0x%02X, got 0x%02X", data[4], checksum);
        return ESP_ERR_INVALID_CRC;
    } else {
        /* Checksum OK */
    }

    /* Convert humidity (MISRA Rule 10.3 - explicit cast) */
    uint16_t humid_raw = ((uint16_t)data[0] << 8) | (uint16_t)data[1];
    *humidity = (float)humid_raw / 10.0f;

    /* Convert temperature */
    uint16_t temp_raw = ((uint16_t)(data[2] & 0x7FU) << 8) | (uint16_t)data[3];
    *temperature = (float)temp_raw / 10.0f;

    /* Check sign bit for negative temperature */
    if ((data[2] & 0x80U) != 0U) {
        *temperature = -*temperature;
    } else {
        /* Positive temperature */
    }

    return ESP_OK;
}
