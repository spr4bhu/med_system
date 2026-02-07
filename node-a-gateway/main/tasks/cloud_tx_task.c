#include "cloud_tx_task.h"
#include "config.h"
#include "utils/data_structures.h"
#include "message_types.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include <string.h>
#include <stdio.h>

static const char* TAG = "CLOUD_TX";

/* WiFi event handler */
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data) {
    (void)arg;
    (void)event_data;

    if ((event_base == WIFI_EVENT) && (event_id == WIFI_EVENT_STA_START)) {
        (void)esp_wifi_connect();
    } else if ((event_base == WIFI_EVENT) && (event_id == WIFI_EVENT_STA_DISCONNECTED)) {
        ESP_LOGW(TAG, "WiFi disconnected, reconnecting...");
        (void)esp_wifi_connect();
    } else if ((event_base == IP_EVENT) && (event_id == IP_EVENT_STA_GOT_IP)) {
        ESP_LOGI(TAG, "Got IP address");
    } else {
        /* Other events */
    }
}

/* Initialize WiFi */
static esp_err_t wifi_init(void) {
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_err_t ret = esp_wifi_init(&cfg);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "WiFi init failed: %d", ret);
        return ret;
    } else {
        /* WiFi init OK */
    }

    /* Register event handlers */
    ret = esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL);
    if (ret != ESP_OK) {
        return ret;
    } else {
        /* Event handler registered */
    }

    ret = esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL);
    if (ret != ESP_OK) {
        return ret;
    } else {
        /* Event handler registered */
    }

    /* Configure WiFi */
    wifi_config_t wifi_config;
    (void)memset(&wifi_config, 0, sizeof(wifi_config));
    (void)strncpy((char*)wifi_config.sta.ssid, WIFI_SSID, sizeof(wifi_config.sta.ssid) - 1U);
    (void)strncpy((char*)wifi_config.sta.password, WIFI_PASSWORD, sizeof(wifi_config.sta.password) - 1U);

    ret = esp_wifi_set_mode(WIFI_MODE_STA);
    if (ret != ESP_OK) {
        return ret;
    } else {
        /* Mode set */
    }

    ret = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    if (ret != ESP_OK) {
        return ret;
    } else {
        /* Config set */
    }

    ret = esp_wifi_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "WiFi start failed: %d", ret);
        return ret;
    } else {
        ESP_LOGI(TAG, "WiFi started, connecting to %s", WIFI_SSID);
    }

    return ESP_OK;
}

/* HTTP POST to cloud */
static esp_err_t send_to_cloud(const char* json_payload) {
    if (json_payload == NULL) {
        return ESP_ERR_INVALID_ARG;
    } else {
        /* Valid pointer */
    }

    esp_http_client_config_t config;
    (void)memset(&config, 0, sizeof(config));
    config.url = CLOUD_SERVER_URL;
    config.method = HTTP_METHOD_POST;
    config.timeout_ms = 5000;

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        return ESP_FAIL;
    } else {
        /* Client initialized */
    }

    /* Set headers */
    (void)esp_http_client_set_header(client, "Content-Type", "application/json");
    (void)esp_http_client_set_header(client, "X-API-Key", API_KEY);

    /* Set POST data */
    esp_err_t ret = esp_http_client_set_post_field(client, json_payload, (int)strlen(json_payload));
    if (ret != ESP_OK) {
        (void)esp_http_client_cleanup(client);
        return ret;
    } else {
        /* POST field set */
    }

    /* Perform HTTP request */
    ret = esp_http_client_perform(client);

    if (ret == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "HTTP POST status: %d", status_code);

        if ((status_code >= 200) && (status_code < 300)) {
            ret = ESP_OK;
        } else {
            ret = ESP_FAIL;
        }
    } else {
        ESP_LOGE(TAG, "HTTP POST failed: %d", ret);
    }

    (void)esp_http_client_cleanup(client);
    return ret;
}

void cloud_tx_task(void* pvParameters) {
    (void)pvParameters;  /* Unused parameter (MISRA Rule 2.7) */

    ESP_LOGI(TAG, "Cloud TX task started");

    /* Initialize WiFi */
    esp_err_t ret = wifi_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "WiFi initialization failed");
        vTaskDelete(NULL);
        return;
    } else {
        /* WiFi initialized */
    }

    /* Wait for WiFi connection */
    vTaskDelay(pdMS_TO_TICKS(5000U));

    sensor_data_t sensor_data;
    char json_buffer[512];

    while (1) {
        /* Get latest sensor data */
        BaseType_t queue_ret = xQueuePeek(
            sensor_data_queue,
            &sensor_data,
            pdMS_TO_TICKS(CLOUD_TX_INTERVAL_MS)
        );

        if (queue_ret == pdPASS) {
            /* Check emergency events */
            EventBits_t events = xEventGroupGetBits(emergency_event_group);

            /* Build JSON payload */
            (void)snprintf(json_buffer, sizeof(json_buffer),
                "{\"heart_rate\":%.1f,\"temperature\":%.1f,"
                "\"accel_x\":%.2f,\"accel_y\":%.2f,\"accel_z\":%.2f,"
                "\"posture\":%d,\"fall_detected\":%d,\"sos_pressed\":%d,"
                "\"vitals_abnormal\":%d,\"timestamp\":%lu}",
                sensor_data.heart_rate_bpm,
                sensor_data.temperature_c,
                sensor_data.accel_x,
                sensor_data.accel_y,
                sensor_data.accel_z,
                (int)sensor_data.posture,
                ((events & FALL_DETECTED_BIT) != 0U) ? 1 : 0,
                ((events & SOS_PRESSED_BIT) != 0U) ? 1 : 0,
                ((events & VITALS_ABNORMAL_BIT) != 0U) ? 1 : 0,
                (unsigned long)sensor_data.timestamp_ms
            );

            /* Send to cloud */
            ret = send_to_cloud(json_buffer);

            if (ret == ESP_OK) {
                ESP_LOGI(TAG, "Data sent to cloud successfully");

                /* Clear one-time event bits after successful transmission */
                (void)xEventGroupClearBits(emergency_event_group, FALL_DETECTED_BIT | SOS_PRESSED_BIT);
            } else {
                ESP_LOGW(TAG, "Failed to send data to cloud: %d", ret);
            }
        } else {
            ESP_LOGW(TAG, "No sensor data available for transmission");
        }

        /* Sleep for transmission interval */
        vTaskDelay(pdMS_TO_TICKS(CLOUD_TX_INTERVAL_MS));
    }

    /* Task should never return */
    vTaskDelete(NULL);
}
