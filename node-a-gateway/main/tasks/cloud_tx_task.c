#include "tasks/cloud_tx_task.h"
#include "config.h"
#include "utils/data_structures.h"
#include "hivemq_cert.h"
#include "sms_alert.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_sntp.h"
#include "esp_now.h"
#include "mqtt_client.h"
#include "nvs_flash.h"
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>

static const char *TAG = "MQTT_TASK";

/* Global MQTT state */
static esp_mqtt_client_handle_t s_mqtt_client = NULL;
static bool s_mqtt_connected = false;
static bool s_espnow_initialized = false;

/* Node B MAC address - update to match your hardware */
static uint8_t s_node_b_mac[6] = {0x08, 0xA6, 0xF7, 0xB1, 0x84, 0x28};

/* System state names */
static const char *system_state_names[] = {"IDLE", "NORMAL", "EMERGENCY"};

/* System state tracking */
static system_state_t g_system_state = SYSTEM_STATE_IDLE;
static uint8_t g_emergency_reasons = EMERGENCY_NONE;

/* ========== MQTT Functions ========== */

static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                                int32_t event_id, void *event_data)
{
    (void)handler_args;
    (void)base;
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;

    switch ((int)event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT CONNECTED to %s", MQTT_BROKER_URI);
            s_mqtt_connected = true;
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "MQTT DISCONNECTED");
            s_mqtt_connected = false;
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT ERROR");
            if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
                ESP_LOGE(TAG, "  Transport: 0x%x, TLS: 0x%x",
                         event->error_handle->esp_transport_sock_errno,
                         event->error_handle->esp_tls_last_esp_err);
            } else {
                /* Other error type */
            }
            break;
        default:
            ESP_LOGD(TAG, "MQTT event: %d", (int)event->event_id);
            break;
    }
}

static void mqtt_publish(const char *topic, const char *data)
{
    if (s_mqtt_connected && (s_mqtt_client != NULL)) {
        (void)esp_mqtt_client_publish(s_mqtt_client, topic, data, 0, 0, 0);
        ESP_LOGI(TAG, "MQTT -> %s: %s", topic, data);
    } else {
        /* Not connected */
    }
}

/* ========== WiFi Functions ========== */

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                                int32_t event_id, void *event_data)
{
    (void)arg;
    if ((event_base == WIFI_EVENT) && (event_id == WIFI_EVENT_STA_START)) {
        (void)esp_wifi_connect();
    } else if ((event_base == WIFI_EVENT) && (event_id == WIFI_EVENT_STA_DISCONNECTED)) {
        ESP_LOGW(TAG, "WiFi disconnected, retrying...");
        (void)esp_wifi_connect();
    } else if ((event_base == IP_EVENT) && (event_id == IP_EVENT_STA_GOT_IP)) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "WiFi connected - IP: " IPSTR, IP2STR(&event->ip_info.ip));
    } else {
        /* Other events */
    }
}

static void init_wifi(void)
{
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    uint8_t mac[6];
    (void)esp_wifi_get_mac(WIFI_IF_STA, mac);
    ESP_LOGI(TAG, "WiFi connecting to: %s", WIFI_SSID);
    ESP_LOGW(TAG, "MAC: %02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

static void init_sntp(void)
{
    ESP_LOGI(TAG, "Initializing SNTP...");
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_init();

    time_t now = 0;
    struct tm timeinfo = {0};
    int retry = 0;
    const int retry_count = 15;

    while ((timeinfo.tm_year < (2024 - 1900)) && (++retry < retry_count)) {
        ESP_LOGI(TAG, "Waiting for time sync... (%d/%d)", retry, retry_count);
        vTaskDelay(pdMS_TO_TICKS(2000));
        time(&now);
        localtime_r(&now, &timeinfo);
    }

    if (timeinfo.tm_year < (2024 - 1900)) {
        ESP_LOGW(TAG, "Failed to sync time, TLS may fail!");
    } else {
        char strftime_buf[64];
        (void)strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
        ESP_LOGI(TAG, "Time synchronized: %s", strftime_buf);
    }
}

static void init_mqtt(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_BROKER_URI,
        .session.protocol_ver = MQTT_PROTOCOL_V_3_1_1,
        .network.timeout_ms = 10000,
    };

#if USE_HIVEMQ_CLOUD
    mqtt_cfg.credentials.username = MQTT_USERNAME;
    mqtt_cfg.credentials.authentication.password = MQTT_PASSWORD;
    mqtt_cfg.broker.verification.certificate = hivemq_root_cert_pem;
    mqtt_cfg.broker.verification.use_global_ca_store = false;
    ESP_LOGI(TAG, "MQTT: Using HiveMQ Cloud with TLS");
#endif

    s_mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    (void)esp_mqtt_client_register_event(s_mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    (void)esp_mqtt_client_start(s_mqtt_client);

    ESP_LOGI(TAG, "MQTT connecting to: %s", MQTT_BROKER_URI);
}

/* ========== ESP-NOW Functions ========== */

static void espnow_send_callback(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    if (status == ESP_NOW_SEND_SUCCESS) {
        ESP_LOGI(TAG, "ESP-NOW send to %02X:%02X:%02X:%02X:%02X:%02X OK",
                 mac_addr[0], mac_addr[1], mac_addr[2],
                 mac_addr[3], mac_addr[4], mac_addr[5]);
    } else {
        ESP_LOGW(TAG, "ESP-NOW send FAILED");
    }
}

/* ESP-NOW receive callback for messages from Node B */
static void espnow_recv_callback(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len)
{
    if ((recv_info == NULL) || (data == NULL)) {
        ESP_LOGE(TAG, "NULL pointer in ESP-NOW receive callback");
        return;
    } else {
        /* Valid pointers */
    }

    ESP_LOGI(TAG, "ESP-NOW: Received %d bytes from %02X:%02X:%02X:%02X:%02X:%02X",
             len,
             recv_info->src_addr[0], recv_info->src_addr[1], recv_info->src_addr[2],
             recv_info->src_addr[3], recv_info->src_addr[4], recv_info->src_addr[5]);

    /* Validate message size */
    if (len < 0) {
        ESP_LOGW(TAG, "Invalid packet length: negative");
        return;
    } else if ((size_t)len != sizeof(espnow_emergency_msg_t)) {
        ESP_LOGW(TAG, "Invalid message size %d (expected %zu)", len, sizeof(espnow_emergency_msg_t));
        return;
    } else {
        /* Valid size */
    }

    /* Parse emergency message from Node B */
    espnow_emergency_msg_t msg;
    (void)memcpy(&msg, data, sizeof(espnow_emergency_msg_t));

    if (msg.msg_type == 1U) {
        ESP_LOGW(TAG, "========================================");
        ESP_LOGW(TAG, "SECURITY EVENT FROM NODE B!");
        ESP_LOGW(TAG, "  Reasons: 0x%02X", msg.emergency_reasons);
        ESP_LOGW(TAG, "  Posture: %s", msg.posture);
        ESP_LOGW(TAG, "  Value: %.1f", (double)msg.sensor_value);
        ESP_LOGW(TAG, "========================================");
    } else {
        ESP_LOGI(TAG, "ESP-NOW: msg_type=%d (not emergency)", msg.msg_type);
    }
}

static void init_espnow(void)
{
    /* Get current WiFi channel (set by AP connection) */
    uint8_t primary_channel = 0;
    wifi_second_chan_t secondary_channel = WIFI_SECOND_CHAN_NONE;
    ESP_ERROR_CHECK(esp_wifi_get_channel(&primary_channel, &secondary_channel));

    ESP_LOGI(TAG, "WiFi connected on channel %d - using for ESP-NOW", primary_channel);

    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_send_cb(espnow_send_callback));
    ESP_ERROR_CHECK(esp_now_register_recv_cb(espnow_recv_callback));

    esp_now_peer_info_t peer_info = {
        .channel = primary_channel,  /* Use actual WiFi channel */
        .ifidx = WIFI_IF_STA,
        .encrypt = false,
    };
    (void)memcpy(peer_info.peer_addr, s_node_b_mac, 6);

    ESP_ERROR_CHECK(esp_now_add_peer(&peer_info));
    s_espnow_initialized = true;

    ESP_LOGI(TAG, "ESP-NOW ready on channel %d, peer: %02X:%02X:%02X:%02X:%02X:%02X",
             primary_channel,
             s_node_b_mac[0], s_node_b_mac[1], s_node_b_mac[2],
             s_node_b_mac[3], s_node_b_mac[4], s_node_b_mac[5]);
}

static void espnow_send_emergency_alert(uint8_t emergency_type, const char *posture, float value)
{
    (void)emergency_type;

    if (!s_espnow_initialized) {
        ESP_LOGW(TAG, "ESP-NOW not initialized, skipping alert");
        return;
    } else {
        /* Initialized */
    }

    espnow_emergency_msg_t msg = {
        .msg_type = 1U,
        .emergency_reasons = g_emergency_reasons,
        .sensor_value = value,
    };
    (void)strncpy(msg.posture, posture, sizeof(msg.posture) - 1U);

    ESP_LOGW(TAG, "ESP-NOW: Sending emergency to Node B (reasons: 0x%02X)", msg.emergency_reasons);

    esp_err_t result = esp_now_send(s_node_b_mac, (const uint8_t *)&msg, sizeof(msg));
    if (result == ESP_OK) {
        ESP_LOGW(TAG, "ESP-NOW send queued OK");
    } else {
        ESP_LOGE(TAG, "ESP-NOW send failed: %s", esp_err_to_name(result));
    }
}

/* ========== Emergency Reason String ========== */

static void get_emergency_reasons_string(uint8_t reasons, char *buffer, size_t buffer_size)
{
    if (reasons == (uint8_t)EMERGENCY_NONE) {
        (void)snprintf(buffer, buffer_size, "NONE");
        return;
    } else {
        /* Has reasons */
    }

    buffer[0] = '\0';
    bool first = true;

    if ((reasons & (uint8_t)EMERGENCY_FALL) != 0U) {
        (void)strncat(buffer, "FALL", buffer_size - strlen(buffer) - 1U);
        first = false;
    } else { /* No fall */ }
    if ((reasons & (uint8_t)EMERGENCY_BUTTON) != 0U) {
        if (!first) { (void)strncat(buffer, "+", buffer_size - strlen(buffer) - 1U); }
        (void)strncat(buffer, "BUTTON", buffer_size - strlen(buffer) - 1U);
        first = false;
    } else { /* No button */ }
    if ((reasons & (uint8_t)EMERGENCY_HIGH_TEMP) != 0U) {
        if (!first) { (void)strncat(buffer, "+", buffer_size - strlen(buffer) - 1U); }
        (void)strncat(buffer, "HIGH_TEMP", buffer_size - strlen(buffer) - 1U);
        first = false;
    } else { /* No high temp */ }
    if ((reasons & (uint8_t)EMERGENCY_HIGH_HR) != 0U) {
        if (!first) { (void)strncat(buffer, "+", buffer_size - strlen(buffer) - 1U); }
        (void)strncat(buffer, "HIGH_HR", buffer_size - strlen(buffer) - 1U);
    } else { /* No high HR */ }
}

/* ========== MQTT Task ========== */

void mqtt_task(void *pvParameters)
{
    (void)pvParameters;  /* MISRA Rule 2.7 */

    ESP_LOGI(TAG, "MQTT task started");

    /* Initialize WiFi */
    init_wifi();
    ESP_LOGI(TAG, "Waiting for WiFi...");
    vTaskDelay(pdMS_TO_TICKS(3000));

    /* Initialize SNTP (needed for TLS) */
#if USE_HIVEMQ_CLOUD
    init_sntp();
#endif

    /* Initialize MQTT */
    init_mqtt();
    ESP_LOGI(TAG, "Waiting for MQTT...");
    vTaskDelay(pdMS_TO_TICKS(2000));

    /* Initialize ESP-NOW after WiFi is up */
    init_espnow();

    /* Publish online status */
    mqtt_publish("node_a/status", "ONLINE");

    sensor_data_t sensor_data;
    uint32_t last_mqtt_publish_ms = 0U;
    uint8_t last_emergency_reasons = 0U;

    while (1) {
        uint32_t current_time = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);

        /* Check event group for emergencies */
        EventBits_t events = xEventGroupGetBits(emergency_event_group);

        if ((events & FALL_DETECTED_BIT) != 0U) {
            g_system_state = SYSTEM_STATE_EMERGENCY;
            g_emergency_reasons |= (uint8_t)EMERGENCY_FALL;

            char fall_msg[128];
            (void)snprintf(fall_msg, sizeof(fall_msg),
                     "{\"type\":\"fall\",\"state\":\"EMERGENCY\"}");
            mqtt_publish("node_a/emergency", fall_msg);
            mqtt_publish("node_a/state", "EMERGENCY");

            espnow_send_emergency_alert((uint8_t)EMERGENCY_FALL, "UNKNOWN", 0.0f);

            send_emergency_sms("FALL_DETECTED", "fall detected");

            (void)xEventGroupClearBits(emergency_event_group, FALL_DETECTED_BIT);
        } else {
            /* No fall */
        }

        if ((events & SOS_PRESSED_BIT) != 0U) {
            g_system_state = SYSTEM_STATE_EMERGENCY;
            g_emergency_reasons |= (uint8_t)EMERGENCY_BUTTON;

            char button_msg[128];
            (void)snprintf(button_msg, sizeof(button_msg),
                     "{\"type\":\"button\",\"state\":\"EMERGENCY\"}");
            mqtt_publish("node_a/emergency", button_msg);
            mqtt_publish("node_a/state", "EMERGENCY");

            espnow_send_emergency_alert((uint8_t)EMERGENCY_BUTTON, "UNKNOWN", 0.0f);

            send_emergency_sms("BUTTON_PRESS", "emergency button pressed");

            (void)xEventGroupClearBits(emergency_event_group, SOS_PRESSED_BIT);
        } else {
            /* No SOS */
        }

        if ((events & VITALS_ABNORMAL_BIT) != 0U) {
            g_system_state = SYSTEM_STATE_EMERGENCY;
            g_emergency_reasons |= (uint8_t)EMERGENCY_HIGH_TEMP;

            mqtt_publish("node_a/state", "EMERGENCY");
        } else {
            /* Vitals normal */
        }

        /* Publish sensor data every 5 seconds */
        if ((current_time - last_mqtt_publish_ms) >= MQTT_PUBLISH_INTERVAL_MS) {
            BaseType_t queue_ret = xQueuePeek(sensor_data_queue, &sensor_data, 0);
            if (queue_ret == pdPASS) {
                char emergency_str[64];
                get_emergency_reasons_string(g_emergency_reasons, emergency_str, sizeof(emergency_str));

                bool emergency_changed = (g_emergency_reasons != last_emergency_reasons);
                last_emergency_reasons = g_emergency_reasons;

                char sensor_msg[350];
                (void)snprintf(sensor_msg, sizeof(sensor_msg),
                         "{\"temp\":%.1f,\"humidity\":%d,\"hr_voltage\":%.0f,"
                         "\"system_state\":\"%s\",\"emergency_type\":\"%s\"}",
                         sensor_data.temperature.temperature,
                         sensor_data.temperature.humidity,
                         sensor_data.heart_rate.voltage_mv,
                         system_state_names[g_system_state],
                         emergency_str);

                if (emergency_changed || (g_emergency_reasons == 0U)) {
                    mqtt_publish("node_a/sensors", sensor_msg);
                } else {
                    /* Skip to avoid spam during ongoing emergency */
                }
            } else {
                /* No data available */
            }
            last_mqtt_publish_ms = current_time;
        } else {
            /* Not time to publish */
        }

        /* Clear emergency if no active reasons */
        if ((g_emergency_reasons != 0U) && ((events & (FALL_DETECTED_BIT | SOS_PRESSED_BIT | VITALS_ABNORMAL_BIT)) == 0U)) {
            g_emergency_reasons = (uint8_t)EMERGENCY_NONE;
            g_system_state = SYSTEM_STATE_NORMAL;
            mqtt_publish("node_a/state", system_state_names[g_system_state]);
        } else {
            /* Emergency still active or no emergency */
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }

    vTaskDelete(NULL);
}
