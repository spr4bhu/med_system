#include "mpu6050.h"
#include "config.h"
#include "utils/data_structures.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include <string.h>

static const char* TAG = "MPU6050";

/* MPU6050 Register Addresses */
#define MPU6050_PWR_MGMT_1   0x6BU
#define MPU6050_ACCEL_XOUT_H 0x3BU
#define MPU6050_GYRO_XOUT_H  0x43U
#define MPU6050_WHO_AM_I     0x75U

/* Scale factors */
#define ACCEL_SCALE_FACTOR   16384.0f  /* ±2g range */
#define GYRO_SCALE_FACTOR    131.0f    /* ±250°/s range */

/* Timeout */
#define I2C_TIMEOUT_MS       1000

/* Helper function to write register */
static esp_err_t mpu6050_write_reg(uint8_t reg_addr, uint8_t data) {
    uint8_t write_buf[2];
    write_buf[0] = reg_addr;
    write_buf[1] = data;

    esp_err_t ret = i2c_master_write_to_device(
        I2C_MASTER_NUM,
        MPU6050_I2C_ADDR,
        write_buf,
        2U,
        pdMS_TO_TICKS(I2C_TIMEOUT_MS)
    );

    return ret;
}

/* Helper function to read registers */
static esp_err_t mpu6050_read_regs(uint8_t reg_addr, uint8_t* data, size_t len) {
    if (data == NULL) {
        return ESP_ERR_INVALID_ARG;
    } else {
        /* Valid pointer */
    }

    esp_err_t ret = i2c_master_write_read_device(
        I2C_MASTER_NUM,
        MPU6050_I2C_ADDR,
        &reg_addr,
        1U,
        data,
        len,
        pdMS_TO_TICKS(I2C_TIMEOUT_MS)
    );

    return ret;
}

esp_err_t mpu6050_init(void) {
    ESP_LOGI(TAG, "Initializing MPU6050...");

    /* Configure I2C master */
    i2c_config_t conf;
    (void)memset(&conf, 0, sizeof(conf));
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_MASTER_SDA_IO;
    conf.scl_io_num = I2C_MASTER_SCL_IO;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ;

    esp_err_t ret = i2c_param_config(I2C_MASTER_NUM, &conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C param config failed: %d", ret);
        return ret;
    } else {
        /* Success */
    }

    /* ESP-IDF API: last parameter is int intr_alloc_flags; use 0 to match type (MISRA 10.x) */
    ret = i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0U, 0U, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C driver install failed: %d", ret);
        return ret;
    } else {
        /* Success */
    }

    /* Verify device WHO_AM_I register */
    uint8_t who_am_i = 0U;
    ret = mpu6050_read_regs(MPU6050_WHO_AM_I, &who_am_i, 1U);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read WHO_AM_I: %d", ret);
        return ret;
    } else {
        ESP_LOGI(TAG, "WHO_AM_I: 0x%02X", who_am_i);
    }

    /* Wake up MPU6050 (write 0x00 to PWR_MGMT_1) */
    ret = mpu6050_write_reg(MPU6050_PWR_MGMT_1, 0x00U);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to wake up device: %d", ret);
        return ret;
    } else {
        ESP_LOGI(TAG, "MPU6050 initialized successfully");
    }

    return ESP_OK;
}

esp_err_t mpu6050_read_accel(float* ax, float* ay, float* az) {
    if ((ax == NULL) || (ay == NULL) || (az == NULL)) {
        return ESP_ERR_INVALID_ARG;
    } else {
        /* Valid pointers */
    }

    /* Take I2C mutex (MISRA Rule 17.7) */
    if (xSemaphoreTake(i2c_mutex, pdMS_TO_TICKS(I2C_TIMEOUT_MS)) != pdTRUE) {
        ESP_LOGW(TAG, "Failed to acquire I2C mutex");
        return ESP_ERR_TIMEOUT;
    } else {
        /* Mutex acquired */
    }

    /* Read 6 bytes from accelerometer registers */
    uint8_t data[6];
    esp_err_t ret = mpu6050_read_regs(MPU6050_ACCEL_XOUT_H, data, 6U);

    xSemaphoreGive(i2c_mutex);

    if (ret != ESP_OK) {
        return ret;
    } else {
        /* Read successful */
    }

    /* Convert to int16_t (MISRA Rule 10.3 - explicit cast) */
    int16_t accel_x = (int16_t)(((uint16_t)data[0] << 8) | (uint16_t)data[1]);
    int16_t accel_y = (int16_t)(((uint16_t)data[2] << 8) | (uint16_t)data[3]);
    int16_t accel_z = (int16_t)(((uint16_t)data[4] << 8) | (uint16_t)data[5]);

    /* Convert to g (MISRA Rule 10.4 - explicit conversion) */
    *ax = (float)accel_x / ACCEL_SCALE_FACTOR;
    *ay = (float)accel_y / ACCEL_SCALE_FACTOR;
    *az = (float)accel_z / ACCEL_SCALE_FACTOR;

    return ESP_OK;
}

esp_err_t mpu6050_read_gyro(float* gx, float* gy, float* gz) {
    if ((gx == NULL) || (gy == NULL) || (gz == NULL)) {
        return ESP_ERR_INVALID_ARG;
    } else {
        /* Valid pointers */
    }

    if (xSemaphoreTake(i2c_mutex, pdMS_TO_TICKS(I2C_TIMEOUT_MS)) != pdTRUE) {
        ESP_LOGW(TAG, "Failed to acquire I2C mutex");
        return ESP_ERR_TIMEOUT;
    } else {
        /* Mutex acquired */
    }

    uint8_t data[6];
    esp_err_t ret = mpu6050_read_regs(MPU6050_GYRO_XOUT_H, data, 6U);

    xSemaphoreGive(i2c_mutex);

    if (ret != ESP_OK) {
        return ret;
    } else {
        /* Read successful */
    }

    int16_t gyro_x = (int16_t)(((uint16_t)data[0] << 8) | (uint16_t)data[1]);
    int16_t gyro_y = (int16_t)(((uint16_t)data[2] << 8) | (uint16_t)data[3]);
    int16_t gyro_z = (int16_t)(((uint16_t)data[4] << 8) | (uint16_t)data[5]);

    *gx = (float)gyro_x / GYRO_SCALE_FACTOR;
    *gy = (float)gyro_y / GYRO_SCALE_FACTOR;
    *gz = (float)gyro_z / GYRO_SCALE_FACTOR;

    return ESP_OK;
}
