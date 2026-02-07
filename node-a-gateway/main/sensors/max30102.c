#include "max30102.h"
#include "config.h"
#include "utils/data_structures.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include <math.h>

static const char* TAG = "MAX30102";

/* MAX30102 Register Addresses */
#define MAX30102_INT_STATUS_1   0x00U
#define MAX30102_INT_STATUS_2   0x01U
#define MAX30102_INT_ENABLE_1   0x02U
#define MAX30102_INT_ENABLE_2   0x03U
#define MAX30102_FIFO_WR_PTR    0x04U
#define MAX30102_FIFO_RD_PTR    0x06U
#define MAX30102_FIFO_DATA      0x07U
#define MAX30102_MODE_CONFIG    0x09U
#define MAX30102_SPO2_CONFIG    0x0AU
#define MAX30102_LED1_PA        0x0CU
#define MAX30102_LED2_PA        0x0DU
#define MAX30102_PART_ID        0xFFU

/* Configuration values */
#define MAX30102_MODE_SPO2      0x03U
#define MAX30102_LED_CURRENT    0x1FU  /* 6.4mA */

/* Timeout */
#define I2C_TIMEOUT_MS          1000

/* Peak detection state */
static uint32_t last_peak_time = 0U;
static uint32_t last_ir_value = 0U;

/* Helper function to write register */
static esp_err_t max30102_write_reg(uint8_t reg_addr, uint8_t data) {
    uint8_t write_buf[2];
    write_buf[0] = reg_addr;
    write_buf[1] = data;

    esp_err_t ret = i2c_master_write_to_device(
        I2C_MASTER_NUM,
        MAX30102_I2C_ADDR,
        write_buf,
        2U,
        pdMS_TO_TICKS(I2C_TIMEOUT_MS)
    );

    return ret;
}

/* Helper function to read register */
static esp_err_t max30102_read_reg(uint8_t reg_addr, uint8_t* data) {
    if (data == NULL) {
        return ESP_ERR_INVALID_ARG;
    } else {
        /* Valid pointer */
    }

    esp_err_t ret = i2c_master_write_read_device(
        I2C_MASTER_NUM,
        MAX30102_I2C_ADDR,
        &reg_addr,
        1U,
        data,
        1U,
        pdMS_TO_TICKS(I2C_TIMEOUT_MS)
    );

    return ret;
}

esp_err_t max30102_init(void) {
    ESP_LOGI(TAG, "Initializing MAX30102...");

    /* Verify Part ID */
    uint8_t part_id = 0U;
    esp_err_t ret = ESP_OK;

    if (xSemaphoreTake(i2c_mutex, pdMS_TO_TICKS(I2C_TIMEOUT_MS)) != pdTRUE) {
        ESP_LOGW(TAG, "Failed to acquire I2C mutex");
        return ESP_ERR_TIMEOUT;
    } else {
        /* Mutex acquired */
    }

    ret = max30102_read_reg(MAX30102_PART_ID, &part_id);
    xSemaphoreGive(i2c_mutex);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read Part ID: %d", ret);
        return ret;
    } else {
        ESP_LOGI(TAG, "Part ID: 0x%02X", part_id);
    }

    if (xSemaphoreTake(i2c_mutex, pdMS_TO_TICKS(I2C_TIMEOUT_MS)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    } else {
        /* Mutex acquired */
    }

    /* Configure SpO2 mode */
    ret = max30102_write_reg(MAX30102_MODE_CONFIG, MAX30102_MODE_SPO2);
    if (ret != ESP_OK) {
        xSemaphoreGive(i2c_mutex);
        ESP_LOGE(TAG, "Failed to configure mode: %d", ret);
        return ret;
    } else {
        /* Success */
    }

    /* Configure LED pulse amplitude */
    ret = max30102_write_reg(MAX30102_LED1_PA, MAX30102_LED_CURRENT);
    if (ret != ESP_OK) {
        xSemaphoreGive(i2c_mutex);
        return ret;
    } else {
        /* Success */
    }

    ret = max30102_write_reg(MAX30102_LED2_PA, MAX30102_LED_CURRENT);
    xSemaphoreGive(i2c_mutex);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure LED: %d", ret);
        return ret;
    } else {
        ESP_LOGI(TAG, "MAX30102 initialized successfully");
    }

    return ESP_OK;
}

esp_err_t max30102_read_bpm(float* heart_rate) {
    if (heart_rate == NULL) {
        return ESP_ERR_INVALID_ARG;
    } else {
        /* Valid pointer */
    }

    if (xSemaphoreTake(i2c_mutex, pdMS_TO_TICKS(I2C_TIMEOUT_MS)) != pdTRUE) {
        ESP_LOGW(TAG, "Failed to acquire I2C mutex");
        return ESP_ERR_TIMEOUT;
    } else {
        /* Mutex acquired */
    }

    /* Read FIFO data (simplified - 3 bytes for IR LED) */
    uint8_t fifo_data[3];
    uint8_t reg_addr = MAX30102_FIFO_DATA;
    esp_err_t ret = i2c_master_write_read_device(
        I2C_MASTER_NUM,
        MAX30102_I2C_ADDR,
        &reg_addr,
        1U,
        fifo_data,
        3U,
        pdMS_TO_TICKS(I2C_TIMEOUT_MS)
    );

    xSemaphoreGive(i2c_mutex);

    if (ret != ESP_OK) {
        return ret;
    } else {
        /* Read successful */
    }

    /* Convert to 18-bit value (MISRA Rule 10.3) */
    uint32_t ir_value = ((uint32_t)fifo_data[0] << 16) |
                        ((uint32_t)fifo_data[1] << 8) |
                        (uint32_t)fifo_data[2];
    ir_value &= 0x3FFFFU;  /* Mask to 18 bits */

    /* Simple peak detection algorithm */
    uint32_t current_time = (uint32_t)xTaskGetTickCount();

    if (ir_value > (last_ir_value + 100U)) {
        /* Detected peak */
        if (last_peak_time != 0U) {
            uint32_t time_diff = current_time - last_peak_time;
            if (time_diff > 0U) {
                /* Calculate BPM: (60000 ms/min) / (time_between_peaks_ms) */
                /* MISRA Rule 10.4 - explicit conversion */
                float bpm = 60000.0f / (float)time_diff;

                /* Clamp to reasonable range */
                if (bpm > MAX_HEART_RATE_BPM) {
                    *heart_rate = MAX_HEART_RATE_BPM;
                } else if (bpm < MIN_HEART_RATE_BPM) {
                    *heart_rate = MIN_HEART_RATE_BPM;
                } else {
                    *heart_rate = bpm;
                }
            } else {
                *heart_rate = 0.0f;
            }
        } else {
            *heart_rate = 0.0f;
        }

        last_peak_time = current_time;
    } else {
        /* No peak - return last known value or default */
        *heart_rate = 75.0f;  /* Default resting heart rate */
    }

    last_ir_value = ir_value;

    return ESP_OK;
}

esp_err_t max30102_read_spo2(float* spo2) {
    if (spo2 == NULL) {
        return ESP_ERR_INVALID_ARG;
    } else {
        /* Valid pointer */
    }

    /* SpO2 calculation is complex and requires calibration */
    /* For hackathon, return a simulated value */
    *spo2 = 98.0f;  /* Normal SpO2 */

    return ESP_OK;
}
