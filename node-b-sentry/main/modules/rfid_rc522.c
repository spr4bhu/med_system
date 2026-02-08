#include "rfid_rc522.h"
#include "config.h"
#include "utils/data_structures.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "RFID_RC522";

static spi_device_handle_t spi_device;

/* Authorized and Unauthorized RFID tags database */
static const rfid_entry_t rfid_database[] = {
    /* Authorized tags */
    {{0x72U, 0x0BU, 0xA7U, 0x05U}, "Authorized Tag 1", true},

    /* Unauthorized tags */
    {{0x52U, 0x81U, 0xA2U, 0x5CU}, "Unauthorized Tag 1", false},
};

#define RFID_DATABASE_SIZE (sizeof(rfid_database) / sizeof(rfid_database[0]))

/* ========== Register Read/Write ========== */

static void rc522_write_reg(uint8_t reg, uint8_t val)
{
    uint8_t tx_data[2] = {(uint8_t)((reg << 1) & 0x7EU), val};
    spi_transaction_t trans = {
        .length = 16U,
        .tx_buffer = tx_data,
    };

    if (xSemaphoreTake(spi_mutex, pdMS_TO_TICKS(1000U)) == pdTRUE) {
        (void)spi_device_transmit(spi_device, &trans);
        (void)xSemaphoreGive(spi_mutex);
    } else {
        ESP_LOGW(TAG, "Failed to acquire SPI mutex for write");
    }
}

static uint8_t rc522_read_reg(uint8_t reg)
{
    uint8_t tx_data[2] = {(uint8_t)(((reg << 1) & 0x7EU) | 0x80U), 0x00U};
    uint8_t rx_data[2] = {0U, 0U};
    spi_transaction_t trans = {
        .length = 16U,
        .tx_buffer = tx_data,
        .rx_buffer = rx_data,
    };

    if (xSemaphoreTake(spi_mutex, pdMS_TO_TICKS(1000U)) == pdTRUE) {
        (void)spi_device_transmit(spi_device, &trans);
        (void)xSemaphoreGive(spi_mutex);
    } else {
        ESP_LOGW(TAG, "Failed to acquire SPI mutex for read");
    }

    return rx_data[1];
}

static void rc522_set_bitmask(uint8_t reg, uint8_t mask)
{
    uint8_t tmp = rc522_read_reg(reg);
    rc522_write_reg(reg, tmp | mask);
}

static void rc522_clear_bitmask(uint8_t reg, uint8_t mask)
{
    uint8_t tmp = rc522_read_reg(reg);
    rc522_write_reg(reg, (uint8_t)(tmp & ((uint8_t)~mask)));
}

static void rc522_antenna_on(void)
{
    uint8_t temp = rc522_read_reg(RC522_REG_TX_CONTROL);
    if ((temp & 0x03U) == 0U) {
        rc522_set_bitmask(RC522_REG_TX_CONTROL, 0x03U);
    } else {
        /* Antenna already on */
    }
}

/* ========== RC522 Init ========== */

static void rc522_chip_init(void)
{
    /* Soft reset */
    rc522_write_reg(RC522_REG_COMMAND, RC522_CMD_SOFT_RESET);
    vTaskDelay(pdMS_TO_TICKS(50U));

    /* Timer setup */
    rc522_write_reg(RC522_REG_TIMER_MODE, 0x8DU);
    rc522_write_reg(RC522_REG_TIMER_PRESCALER, 0x3EU);
    rc522_write_reg(RC522_REG_TIMER_RELOAD_L, 30U);
    rc522_write_reg(RC522_REG_TIMER_RELOAD_H, 0U);

    /* TX setup */
    rc522_write_reg(RC522_REG_TX_ASK, 0x40U);
    rc522_write_reg(RC522_REG_MODE, 0x3DU);

    /* Turn on antenna */
    rc522_antenna_on();
}

/* ========== Communication ========== */

static uint8_t rc522_communicate(uint8_t command, uint8_t *send_data, uint8_t send_len,
                                  uint8_t *back_data, uint8_t *back_len, uint8_t *valid_bits)
{
    uint8_t n;
    uint8_t wait_irq = 0x00U;

    switch (command) {
        case RC522_CMD_MF_AUTHENT:
            wait_irq = 0x10U;
            break;
        case RC522_CMD_TRANSCEIVE:
            wait_irq = 0x30U;
            break;
        default:
            break;
    }

    rc522_write_reg(RC522_REG_COM_IRQ, 0x7FU);
    rc522_clear_bitmask(RC522_REG_FIFO_LEVEL, 0x80U);
    rc522_write_reg(RC522_REG_COMMAND, RC522_CMD_IDLE);

    /* Write data to FIFO */
    for (uint8_t i = 0U; i < send_len; i++) {
        rc522_write_reg(RC522_REG_FIFO_DATA, send_data[i]);
    }

    /* Execute command */
    rc522_write_reg(RC522_REG_COMMAND, command);
    if (command == RC522_CMD_TRANSCEIVE) {
        rc522_set_bitmask(RC522_REG_BIT_FRAMING, 0x80U);
    } else {
        /* Not transceive */
    }

    /* Wait for completion */
    n = 100U;
    while (n > 0U) {
        n--;
        uint8_t irq = rc522_read_reg(RC522_REG_COM_IRQ);
        if ((irq & wait_irq) != 0U) {
            break;
        } else if ((irq & 0x01U) != 0U) {
            return RC522_STATUS_TIMEOUT;
        } else {
            /* Keep waiting */
        }
        vTaskDelay(pdMS_TO_TICKS(1U));
    }

    rc522_clear_bitmask(RC522_REG_BIT_FRAMING, 0x80U);

    if (n == 0U) {
        return RC522_STATUS_TIMEOUT;
    } else {
        /* Completed in time */
    }

    /* Check errors */
    uint8_t error = rc522_read_reg(RC522_REG_ERROR);
    if ((error & 0x1BU) != 0U) {
        return RC522_STATUS_ERROR;
    } else {
        /* No errors */
    }

    /* Read response */
    if ((back_data != NULL) && (back_len != NULL)) {
        n = rc522_read_reg(RC522_REG_FIFO_LEVEL);
        *back_len = n;
        for (uint8_t i = 0U; i < n; i++) {
            back_data[i] = rc522_read_reg(RC522_REG_FIFO_DATA);
        }
        if (valid_bits != NULL) {
            *valid_bits = (uint8_t)(rc522_read_reg(RC522_REG_CONTROL) & 0x07U);
        } else {
            /* No valid bits requested */
        }
    } else {
        /* No response buffer */
    }

    return RC522_STATUS_OK;
}

static uint8_t rc522_request(uint8_t *atqa)
{
    uint8_t status;
    uint8_t back_len = 0U;
    uint8_t send_data = PICC_CMD_REQA;

    rc522_write_reg(RC522_REG_BIT_FRAMING, 0x07U);
    status = rc522_communicate(RC522_CMD_TRANSCEIVE, &send_data, 1U, atqa, &back_len, NULL);

    if ((status != RC522_STATUS_OK) || (back_len != 2U)) {
        status = RC522_STATUS_ERROR;
    } else {
        /* Request OK */
    }

    return status;
}

static uint8_t rc522_anticoll(uint8_t *uid)
{
    uint8_t status;
    uint8_t back_len = 0U;
    uint8_t send_data[2] = {PICC_CMD_SEL_CL1, 0x20U};

    rc522_write_reg(RC522_REG_BIT_FRAMING, 0x00U);
    status = rc522_communicate(RC522_CMD_TRANSCEIVE, send_data, 2U, uid, &back_len, NULL);

    if ((status != RC522_STATUS_OK) || (back_len != 5U)) {
        status = RC522_STATUS_ERROR;
    } else {
        /* Anticollision OK */
    }

    return status;
}

/* ========== Public API ========== */

esp_err_t rfid_rc522_init(void)
{
    ESP_LOGI(TAG, "Initializing RC522 RFID reader...");

    /* SPI bus configuration */
    spi_bus_config_t buscfg = {
        .miso_io_num = RC522_MISO_PIN,
        .mosi_io_num = RC522_MOSI_PIN,
        .sclk_io_num = RC522_SCLK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };

    /* SPI device configuration */
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 5000000,  /* 5 MHz */
        .mode = 0,
        .spics_io_num = RC522_CS_PIN,
        .queue_size = 7,
    };

    /* Initialize SPI bus */
    esp_err_t ret = spi_bus_initialize(SPI3_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if ((ret != ESP_OK) && (ret != ESP_ERR_INVALID_STATE)) {
        ESP_LOGE(TAG, "SPI bus init failed: %s", esp_err_to_name(ret));
        return ret;
    } else {
        /* SPI bus ready */
    }

    ret = spi_bus_add_device(SPI3_HOST, &devcfg, &spi_device);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI add device failed: %s", esp_err_to_name(ret));
        return ret;
    } else {
        /* Device added */
    }

    /* Initialize RC522 chip */
    rc522_chip_init();

    /* Verify communication by reading version */
    uint8_t version = rc522_read_reg(RC522_REG_VERSION);
    ESP_LOGI(TAG, "RC522 Version: 0x%02X", version);

    if ((version == 0x00U) || (version == 0xFFU)) {
        ESP_LOGE(TAG, "RC522 not responding (version=0x%02X)", version);
        return ESP_FAIL;
    } else {
        ESP_LOGI(TAG, "RC522 initialized successfully");
    }

    return ESP_OK;
}

rfid_result_t rfid_rc522_check_card(void)
{
    uint8_t atqa[2] = {0U, 0U};
    uint8_t uid[5] = {0U, 0U, 0U, 0U, 0U};
    rfid_result_t result = {false, false, "Unknown"};

    /* Request for a card */
    if (rc522_request(atqa) == RC522_STATUS_OK) {
        /* Anti-collision, get card UID */
        if (rc522_anticoll(uid) == RC522_STATUS_OK) {
            result.detected = true;
            ESP_LOGI(TAG, "RFID Card detected - UID: %02X:%02X:%02X:%02X",
                     uid[0], uid[1], uid[2], uid[3]);

            /* Check against database */
            for (size_t i = 0U; i < RFID_DATABASE_SIZE; i++) {
                if (memcmp(uid, rfid_database[i].uid, 4U) == 0) {
                    result.authorized = rfid_database[i].authorized;
                    result.name = rfid_database[i].name;

                    if (rfid_database[i].authorized) {
                        ESP_LOGI(TAG, "✓ AUTHORIZED - %s", rfid_database[i].name);
                    } else {
                        ESP_LOGW(TAG, "✗ UNAUTHORIZED - %s", rfid_database[i].name);
                    }
                    return result;
                } else {
                    /* Continue checking */
                }
            }

            /* UID not in database - treat as unauthorized */
            ESP_LOGW(TAG, "✗ UNKNOWN TAG - Not in database");
            result.authorized = false;
            result.name = "Unknown (Not Registered)";
        } else {
            /* Anticollision failed */
        }
    } else {
        /* No card present */
    }

    return result;
}
