#include "mpu6050.h"
#include "config.h"
#include "esp_log.h"
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "MPU6050";

/* I2C helper: write single byte to register */
static esp_err_t mpu6050_write_byte(uint8_t reg, uint8_t val)
{
    uint8_t write_buf[2] = {reg, val};
    return i2c_master_write_to_device(I2C_MASTER_NUM, MPU6050_I2C_ADDR,
                                      write_buf, 2, pdMS_TO_TICKS(1000));
}

/* I2C helper: read bytes from register */
static esp_err_t mpu6050_read_bytes(uint8_t reg, uint8_t *data, size_t len)
{
    return i2c_master_write_read_device(I2C_MASTER_NUM, MPU6050_I2C_ADDR,
                                        &reg, 1, data, len, pdMS_TO_TICKS(1000));
}

esp_err_t mpu6050_init(void)
{
    esp_err_t ret;

    /* Initialize I2C master */
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = (uint32_t)I2C_MASTER_FREQ_HZ,
    };

    ret = i2c_param_config(I2C_MASTER_NUM, &conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C config failed: %s", esp_err_to_name(ret));
        return ret;
    } else {
        /* Success */
    }

    ret = i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C driver install failed: %s", esp_err_to_name(ret));
        return ret;
    } else {
        /* Success */
    }

    vTaskDelay(pdMS_TO_TICKS(100));

    /* Wake up MPU6050 */
    ret = mpu6050_write_byte(MPU6050_PWR_MGMT_1, 0x00);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to wake up MPU6050");
        return ret;
    } else {
        /* Device awake */
    }

    vTaskDelay(pdMS_TO_TICKS(100));

    /* Configure sample rate (100Hz) */
    (void)mpu6050_write_byte(MPU6050_SMPLRT_DIV, 0x09);

    /* Configure DLPF (low-pass filter) */
    (void)mpu6050_write_byte(MPU6050_CONFIG_REG, 0x06);

    /* Configure gyroscope (+/-500 deg/s) */
    (void)mpu6050_write_byte(MPU6050_GYRO_CONFIG, 0x08);

    /* Configure accelerometer (+/-4g) */
    (void)mpu6050_write_byte(MPU6050_ACCEL_CONFIG, 0x08);

    ESP_LOGI(TAG, "MPU6050 initialized (SDA=%d, SCL=%d, +/-4g, +/-500dps)",
             I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO);
    return ESP_OK;
}

esp_err_t mpu6050_read(mpu6050_data_t *data)
{
    if (data == NULL) {
        return ESP_ERR_INVALID_ARG;
    } else {
        /* Valid pointer */
    }

    uint8_t raw_data[14];
    esp_err_t ret;

    /* 14-byte burst read: accel(6) + temp(2) + gyro(6) */
    ret = mpu6050_read_bytes(MPU6050_ACCEL_XOUT_H, raw_data, 14);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read sensor data");
        return ret;
    } else {
        /* Read successful */
    }

    /* Parse accelerometer (convert to m/s^2) */
    int16_t accel_x_raw = (int16_t)(((uint16_t)raw_data[0] << 8) | (uint16_t)raw_data[1]);
    int16_t accel_y_raw = (int16_t)(((uint16_t)raw_data[2] << 8) | (uint16_t)raw_data[3]);
    int16_t accel_z_raw = (int16_t)(((uint16_t)raw_data[4] << 8) | (uint16_t)raw_data[5]);

    data->accel_x = ((float)accel_x_raw / ACCEL_SCALE_FACTOR) * GRAVITY;
    data->accel_y = ((float)accel_y_raw / ACCEL_SCALE_FACTOR) * GRAVITY;
    data->accel_z = ((float)accel_z_raw / ACCEL_SCALE_FACTOR) * GRAVITY;

    /* Parse temperature */
    int16_t temp_raw = (int16_t)(((uint16_t)raw_data[6] << 8) | (uint16_t)raw_data[7]);
    data->temperature = ((float)temp_raw / 340.0f) + 36.53f;

    /* Parse gyroscope (convert to deg/s) */
    int16_t gyro_x_raw = (int16_t)(((uint16_t)raw_data[8] << 8) | (uint16_t)raw_data[9]);
    int16_t gyro_y_raw = (int16_t)(((uint16_t)raw_data[10] << 8) | (uint16_t)raw_data[11]);
    int16_t gyro_z_raw = (int16_t)(((uint16_t)raw_data[12] << 8) | (uint16_t)raw_data[13]);

    data->gyro_x = (float)gyro_x_raw / GYRO_SCALE_FACTOR;
    data->gyro_y = (float)gyro_y_raw / GYRO_SCALE_FACTOR;
    data->gyro_z = (float)gyro_z_raw / GYRO_SCALE_FACTOR;

    return ESP_OK;
}
