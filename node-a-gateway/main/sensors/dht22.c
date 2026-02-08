#include "sensors/dht22.h"
#include "config.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "rom/ets_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "DHT11";

static void dht11_set_output(uint8_t level)
{
    (void)gpio_set_direction((gpio_num_t)DHT11_GPIO_PIN, GPIO_MODE_OUTPUT);
    (void)gpio_set_level((gpio_num_t)DHT11_GPIO_PIN, (uint32_t)level);
}

static void dht11_set_input(void)
{
    (void)gpio_set_direction((gpio_num_t)DHT11_GPIO_PIN, GPIO_MODE_INPUT);
}

static uint8_t dht11_read_level(void)
{
    return (uint8_t)gpio_get_level((gpio_num_t)DHT11_GPIO_PIN);
}

/* Wait for GPIO to reach desired level with timeout */
static int dht11_wait_for_level(uint8_t level, int timeout_us)
{
    int elapsed = 0;
    while (dht11_read_level() != level) {
        if (elapsed > timeout_us) {
            return -1;
        } else {
            /* Continue waiting */
        }
        ets_delay_us(1);
        elapsed++;
    }
    return elapsed;
}

/* Read one bit from DHT11 */
static int dht11_read_bit(void)
{
    /* Wait for low period (50us) */
    if (dht11_wait_for_level(0, 60) < 0) {
        return -1;
    } else {
        /* Low period complete */
    }

    /* Wait for high period start */
    if (dht11_wait_for_level(1, 80) < 0) {
        return -1;
    } else {
        /* High period started */
    }

    /* Measure high period: after 40us it's a '1', before is '0' */
    ets_delay_us(40);
    int bit_value = (int)dht11_read_level();

    /* Wait for high period to end */
    (void)dht11_wait_for_level(0, 60);

    return bit_value;
}

esp_err_t dht11_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << DHT11_GPIO_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
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

    /* Set initial state to high and wait for stabilization */
    (void)gpio_set_level((gpio_num_t)DHT11_GPIO_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(1000));

    ESP_LOGI(TAG, "DHT11 initialized on GPIO %d with pull-up", DHT11_GPIO_PIN);
    return ESP_OK;
}

/**
 * Read DHT11 sensor data.
 * Returns: 0 on success, -1 on timeout, -2 on checksum error
 */
int dht11_read(dht11_data_t *data)
{
    if (data == NULL) {
        return -1;
    } else {
        /* Valid pointer */
    }

    uint8_t bytes[5] = {0};
    int bit_value;

    /* Send start signal: pull low for 18ms */
    dht11_set_output(0);
    ets_delay_us(18000);

    /* Pull high for 40us before releasing */
    dht11_set_output(1);
    ets_delay_us(40);

    /* Switch to input mode - DHT11 takes over */
    dht11_set_input();

    /* Wait for DHT11 response (80us low, 80us high, then 80us low) */
    if (dht11_wait_for_level(0, 100) < 0) {
        ESP_LOGD(TAG, "No response (timeout waiting for initial low)");
        return -1;
    } else {
        /* Initial low received */
    }

    if (dht11_wait_for_level(1, 100) < 0) {
        ESP_LOGD(TAG, "No response (timeout waiting for initial high)");
        return -1;
    } else {
        /* Initial high received */
    }

    if (dht11_wait_for_level(0, 100) < 0) {
        ESP_LOGD(TAG, "No response (timeout waiting for data start)");
        return -1;
    } else {
        /* Data start received */
    }

    /* Disable task switching during bit reading (WiFi still runs) */
    vTaskSuspendAll();

    /* Read 40 bits (5 bytes) */
    for (int i = 0; i < 5; i++) {
        for (int j = 7; j >= 0; j--) {
            bit_value = dht11_read_bit();
            if (bit_value < 0) {
                (void)xTaskResumeAll();
                return -1;
            } else {
                bytes[i] |= (uint8_t)((uint8_t)bit_value << (uint8_t)j);
            }
        }
    }

    /* Re-enable task switching */
    (void)xTaskResumeAll();

    /* Verify checksum */
    uint8_t checksum = (uint8_t)(bytes[0] + bytes[1] + bytes[2] + bytes[3]);
    if (checksum != bytes[4]) {
        static uint32_t checksum_error_count = 0;
        checksum_error_count++;
        if ((checksum_error_count % 10U) == 1U) {
            ESP_LOGW(TAG, "Checksum errors (count: %lu, calc=0x%02X recv=0x%02X)",
                     (unsigned long)checksum_error_count, checksum, bytes[4]);
        }
        return -2;
    } else {
        /* Checksum OK */
    }

    data->humidity = bytes[0];
    data->temperature = (float)bytes[2];
    data->crc_ok = 1U;
    data->last_read_ms = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);

    return 0;
}
