#include "hw827.h"
#include "config.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

static const char *TAG = "HW827";

static adc_oneshot_unit_handle_t adc1_handle = NULL;
static adc_cali_handle_t adc1_cali_handle = NULL;

esp_err_t hw827_init(void)
{
    esp_err_t ret;

    /* Initialize ADC1 unit (ESP-IDF 5.x oneshot API) */
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
    };
    ret = adc_oneshot_new_unit(&init_config, &adc1_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize ADC unit: %s", esp_err_to_name(ret));
        return ret;
    } else {
        /* ADC unit initialized */
    }

    /* Configure ADC channel (12-bit resolution, 0-3.3V range) */
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_12,
        .atten = ADC_ATTEN_DB_11,  /* DB_11 = 0-3.3V (0-2450mV range) */
    };
    ret = adc_oneshot_config_channel(adc1_handle, HW827_ADC_CHANNEL, &config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure ADC channel: %s", esp_err_to_name(ret));
        return ret;
    } else {
        /* Channel configured */
    }

    /* Initialize ADC calibration for accurate voltage conversion (ESP-IDF 5.1 line fitting API) */
    adc_cali_line_fitting_config_t cali_config = {
        .unit_id = ADC_UNIT_1,
        .atten = ADC_ATTEN_DB_11,
        .bitwidth = ADC_BITWIDTH_12,
    };
    ret = adc_cali_create_scheme_line_fitting(&cali_config, &adc1_cali_handle);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Calibration scheme unavailable: %s (using raw values)", esp_err_to_name(ret));
        /* Calibration is optional - continue without it */
    } else {
        ESP_LOGI(TAG, "ADC calibration initialized");
    }

    ESP_LOGI(TAG, "HW-827 initialized on GPIO 36 (ADC1_CH0), 12-bit, 0-3.3V");
    return ESP_OK;
}

esp_err_t hw827_read(hw827_data_t *data)
{
    esp_err_t ret;

    if (data == NULL) {
        return ESP_ERR_INVALID_ARG;
    } else {
        /* Valid pointer */
    }

    /* Read raw ADC value */
    int adc_raw;
    ret = adc_oneshot_read(adc1_handle, HW827_ADC_CHANNEL, &adc_raw);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read ADC: %s", esp_err_to_name(ret));
        return ret;
    } else {
        /* Read successful */
    }

    data->adc_raw = (uint16_t)adc_raw;

    /* Convert to voltage if calibration is available */
    if (adc1_cali_handle != NULL) {
        int voltage_mv;
        ret = adc_cali_raw_to_voltage(adc1_cali_handle, adc_raw, &voltage_mv);
        if (ret == ESP_OK) {
            data->voltage_mv = (float)voltage_mv;
        } else {
            /* Fallback to approximate calculation: (raw / 4095) * 3300 mV */
            data->voltage_mv = ((float)adc_raw / 4095.0f) * 3300.0f;
        }
    } else {
        /* No calibration - use approximate calculation */
        data->voltage_mv = ((float)adc_raw / 4095.0f) * 3300.0f;
    }

    return ESP_OK;
}

uint8_t hw827_detect_peak(const hw827_data_t *current, const hw827_data_t *previous)
{
    if ((current == NULL) || (previous == NULL)) {
        return 0U;
    } else {
        /* Valid pointers */
    }

    float voltage_diff = (previous->voltage_mv > current->voltage_mv)
        ? (previous->voltage_mv - current->voltage_mv)
        : (current->voltage_mv - previous->voltage_mv);

    /* Significant change (>200mV) suggests a heartbeat */
    if (voltage_diff > 200.0f) {
        return 1U;
    } else {
        /* No peak */
    }
    return 0U;
}
