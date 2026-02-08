#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "rom/ets_sys.h"

static const char *TAG = "DHT11_SIMPLE";

// Pin Configuration
#define DHT11_GPIO      5

// DHT11 timing constants (microseconds)
#define DHT11_START_SIGNAL      18000  // 18ms low
#define DHT11_WAIT_RESPONSE     40     // 20-40us high
#define DHT11_TIMEOUT           1000   // 1ms timeout for each bit

// Set GPIO as output and drive low or high
static void dht11_set_output(uint8_t level)
{
    gpio_set_direction(DHT11_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(DHT11_GPIO, level);
}

// Set GPIO as input
static void dht11_set_input(void)
{
    gpio_set_direction(DHT11_GPIO, GPIO_MODE_INPUT);
}

// Read GPIO level
static uint8_t dht11_read(void)
{
    return gpio_get_level(DHT11_GPIO);
}

// Wait for GPIO to reach desired level with timeout
static int dht11_wait_for_level(uint8_t level, int timeout_us)
{
    int elapsed = 0;
    while (dht11_read() != level) {
        if (elapsed > timeout_us) {
            return -1;  // Timeout
        }
        ets_delay_us(1);
        elapsed++;
    }
    return elapsed;
}

// Read one bit from DHT11
static int dht11_read_bit(void)
{
    // Wait for low period (50us)
    if (dht11_wait_for_level(0, 60) < 0) {
        return -1;
    }

    // Wait for high period start
    if (dht11_wait_for_level(1, 80) < 0) {
        return -1;
    }

    // Measure high period duration
    ets_delay_us(40);
    int bit_value = dht11_read();

    // Wait for high period to end
    dht11_wait_for_level(0, 60);

    return bit_value;
}

// Read data from DHT11
static esp_err_t dht11_read_data(float *temperature, float *humidity)
{
    uint8_t data[5] = {0};
    int bit_value;

    // Send start signal: pull low for 18ms
    dht11_set_output(0);
    ets_delay_us(DHT11_START_SIGNAL);

    // Pull high for 20-40us
    dht11_set_output(1);
    ets_delay_us(DHT11_WAIT_RESPONSE);

    // Switch to input mode
    dht11_set_input();

    // Wait for DHT11 response (80us low, 80us high)
    if (dht11_wait_for_level(0, 100) < 0) {
        ESP_LOGE(TAG, "No response from DHT11 (timeout waiting for low)");
        return ESP_FAIL;
    }

    if (dht11_wait_for_level(1, 100) < 0) {
        ESP_LOGE(TAG, "No response from DHT11 (timeout waiting for high)");
        return ESP_FAIL;
    }

    if (dht11_wait_for_level(0, 100) < 0) {
        ESP_LOGE(TAG, "No response from DHT11 (timeout waiting for start)");
        return ESP_FAIL;
    }

    // Read 40 bits (5 bytes)
    for (int i = 0; i < 5; i++) {
        for (int j = 7; j >= 0; j--) {
            bit_value = dht11_read_bit();
            if (bit_value < 0) {
                ESP_LOGE(TAG, "Failed to read bit %d of byte %d", j, i);
                return ESP_FAIL;
            }
            data[i] |= (bit_value << j);
        }
    }

    // Verify checksum (sum of first 4 bytes should equal 5th byte)
    uint8_t checksum = data[0] + data[1] + data[2] + data[3];
    if (checksum != data[4]) {
        ESP_LOGE(TAG, "Checksum failed: calculated=0x%02X, received=0x%02X",
                 checksum, data[4]);
        return ESP_FAIL;
    }

    // DHT11 data format:
    // Byte 0: Humidity integer
    // Byte 1: Humidity decimal (always 0 for DHT11)
    // Byte 2: Temperature integer
    // Byte 3: Temperature decimal (always 0 for DHT11)
    // Byte 4: Checksum
    *humidity = data[0];
    *temperature = data[2];

    return ESP_OK;
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting simple DHT11 test");

    // Configure GPIO
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << DHT11_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    // Set initial state to high
    gpio_set_level(DHT11_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(1000));

    ESP_LOGI(TAG, "DHT11 initialized on GPIO %d", DHT11_GPIO);
    ESP_LOGI(TAG, "Reading interval: 2 seconds");

    float temperature, humidity;
    int read_count = 0;
    int success_count = 0;

    while (1) {
        read_count++;

        // Read sensor data
        if (dht11_read_data(&temperature, &humidity) == ESP_OK) {
            success_count++;
            ESP_LOGI(TAG, "Temperature: %.0f °C", temperature);
            ESP_LOGI(TAG, "Humidity: %.0f %%", humidity);
            ESP_LOGI(TAG, "Success rate: %d/%d (%.1f%%)",
                     success_count, read_count,
                     (float)success_count * 100.0f / read_count);
            ESP_LOGI(TAG, "---");
        } else {
            ESP_LOGW(TAG, "Failed to read from DHT11 (attempt %d)", read_count);
            ESP_LOGI(TAG, "Success rate: %d/%d (%.1f%%)",
                     success_count, read_count,
                     (float)success_count * 100.0f / read_count);
        }

        // DHT11 needs at least 2 seconds between readings
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
