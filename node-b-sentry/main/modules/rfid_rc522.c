#include "rfid_rc522.h"
#include "config.h"
#include "utils/data_structures.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include <string.h>

static const char* TAG = "RFID_RC522";

/* RC522 Register Addresses */
#define RC522_REG_COMMAND       0x01U
#define RC522_REG_COMM_IRQ      0x04U
#define RC522_REG_FIFO_DATA     0x09U
#define RC522_REG_FIFO_LEVEL    0x0AU
#define RC522_REG_CONTROL       0x0CU
#define RC522_REG_BIT_FRAMING   0x0DU
#define RC522_REG_MODE          0x11U
#define RC522_REG_TX_CONTROL    0x14U
#define RC522_REG_TX_ASK        0x15U
#define RC522_REG_VERSION       0x37U

/* RC522 Commands */
#define RC522_CMD_IDLE          0x00U
#define RC522_CMD_TRANSCEIVE    0x0CU
#define RC522_CMD_SOFT_RESET    0x0FU

/* PICC Commands */
#define PICC_CMD_REQA           0x26U
#define PICC_CMD_SEL_CL1        0x93U

static spi_device_handle_t spi_device;

/* Helper function to write register */
static esp_err_t rc522_write_reg(uint8_t reg, uint8_t value) {
    uint8_t tx_data[2];
    tx_data[0] = (reg << 1) & 0x7EU;  /* Write mode */
    tx_data[1] = value;

    spi_transaction_t trans;
    (void)memset(&trans, 0, sizeof(trans));
    trans.length = 16U;  /* 2 bytes * 8 bits */
    trans.tx_buffer = tx_data;

    esp_err_t ret = ESP_OK;

    if (xSemaphoreTake(spi_mutex, pdMS_TO_TICKS(1000U)) != pdTRUE) {
        ESP_LOGW(TAG, "Failed to acquire SPI mutex");
        return ESP_ERR_TIMEOUT;
    } else {
        /* Mutex acquired */
    }

    ret = spi_device_transmit(spi_device, &trans);
    xSemaphoreGive(spi_mutex);

    return ret;
}

/* Helper function to read register */
static esp_err_t rc522_read_reg(uint8_t reg, uint8_t* value) {
    if (value == NULL) {
        return ESP_ERR_INVALID_ARG;
    } else {
        /* Valid pointer */
    }

    uint8_t tx_data[2];
    uint8_t rx_data[2];
    tx_data[0] = ((reg << 1) & 0x7EU) | 0x80U;  /* Read mode */
    tx_data[1] = 0x00U;

    spi_transaction_t trans;
    (void)memset(&trans, 0, sizeof(trans));
    trans.length = 16U;
    trans.tx_buffer = tx_data;
    trans.rx_buffer = rx_data;

    esp_err_t ret = ESP_OK;

    if (xSemaphoreTake(spi_mutex, pdMS_TO_TICKS(1000U)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    } else {
        /* Mutex acquired */
    }

    ret = spi_device_transmit(spi_device, &trans);
    xSemaphoreGive(spi_mutex);

    if (ret == ESP_OK) {
        *value = rx_data[1];
    } else {
        /* Read failed */
    }

    return ret;
}

esp_err_t rfid_rc522_init(void) {
    ESP_LOGI(TAG, "Initializing RC522...");

    /* Configure RST pin */
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << RFID_RST_GPIO);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;

    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "GPIO config failed: %d", ret);
        return ret;
    } else {
        /* GPIO configured */
    }

    /* Reset RC522 */
    (void)gpio_set_level(RFID_RST_GPIO, 0);
    vTaskDelay(pdMS_TO_TICKS(10U));
    (void)gpio_set_level(RFID_RST_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(50U));

    /* Configure SPI bus */
    spi_bus_config_t bus_config;
    (void)memset(&bus_config, 0, sizeof(bus_config));
    bus_config.mosi_io_num = SPI_MOSI_GPIO;
    bus_config.miso_io_num = SPI_MISO_GPIO;
    bus_config.sclk_io_num = SPI_SCK_GPIO;
    bus_config.quadwp_io_num = -1;
    bus_config.quadhd_io_num = -1;

    ret = spi_bus_initialize(SPI_HOST_ID, &bus_config, SPI_DMA_DISABLED);
    if ((ret != ESP_OK) && (ret != ESP_ERR_INVALID_STATE)) {
        ESP_LOGE(TAG, "SPI bus init failed: %d", ret);
        return ret;
    } else {
        /* SPI bus initialized */
    }

    /* Add RC522 device to SPI bus */
    spi_device_interface_config_t dev_config;
    (void)memset(&dev_config, 0, sizeof(dev_config));
    dev_config.clock_speed_hz = (int)SPI_CLK_SPEED_HZ;
    dev_config.mode = 0;  /* SPI mode 0 */
    dev_config.spics_io_num = RFID_SS_GPIO;
    dev_config.queue_size = 7;

    ret = spi_bus_add_device(SPI_HOST_ID, &dev_config, &spi_device);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI add device failed: %d", ret);
        return ret;
    } else {
        /* Device added */
    }

    /* Soft reset RC522 */
    ret = rc522_write_reg(RC522_REG_COMMAND, RC522_CMD_SOFT_RESET);
    if (ret != ESP_OK) {
        return ret;
    } else {
        /* Reset command sent */
    }

    vTaskDelay(pdMS_TO_TICKS(50U));

    /* Read version register to verify communication */
    uint8_t version = 0U;
    ret = rc522_read_reg(RC522_REG_VERSION, &version);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read version: %d", ret);
        return ret;
    } else {
        ESP_LOGI(TAG, "RC522 Version: 0x%02X", version);
    }

    /* Enable antenna */
    uint8_t tx_control = 0U;
    ret = rc522_read_reg(RC522_REG_TX_CONTROL, &tx_control);
    if (ret == ESP_OK) {
        if ((tx_control & 0x03U) != 0x03U) {
            ret = rc522_write_reg(RC522_REG_TX_CONTROL, tx_control | 0x03U);
        } else {
            /* Antenna already enabled */
        }
    } else {
        /* Read failed */
    }

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable antenna: %d", ret);
        return ret;
    } else {
        ESP_LOGI(TAG, "RC522 initialized successfully");
    }

    return ESP_OK;
}

esp_err_t rfid_rc522_read_uid(uint32_t* uid) {
    if (uid == NULL) {
        return ESP_ERR_INVALID_ARG;
    } else {
        /* Valid pointer */
    }

    /* Simplified: Return simulated UID for hackathon */
    /* Full implementation would involve REQA, anticollision, and SELECT */
    *uid = 0x12345678U;  /* Simulated authorized UID */

    return ESP_OK;
}

bool rfid_rc522_is_authorized(uint32_t uid) {
    for (size_t i = 0U; i < AUTHORIZED_UID_COUNT; i++) {
        if (uid == AUTHORIZED_UIDS[i]) {
            return true;
        } else {
            /* Continue checking */
        }
    }

    return false;  /* Not found in authorized list */
}
