#include "tasks/mqtt_task.h"
#include "config.h"
#include "utils/data_structures.h"
#include "hivemq_cert.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_sntp.h"
#include "esp_now.h"
#include "mqtt_client.h"
#include "modules/buzzer.h"
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>

static const char *TAG = "MQTT_B";

/* Global MQTT state */
static esp_mqtt_client_handle_t s_mqtt_client = NULL;
static bool s_mqtt_connected = false;

/* Node A MAC address - update to match your hardware */
static uint8_t s_node_a_mac[6] = {0x44, 0x1D, 0x64, 0xF9, 0x53, 0x50};

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

void mqtt_publish_security(const char *topic, const char *data)
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
        vTaskDelay(pdMS_TO_TICKS(2000U));
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

static void espnow_recv_callback(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len)
{
    if ((recv_info == NULL) || (data == NULL)) {
        ESP_LOGE(TAG, "NULL pointer in ESP-NOW callback");
        return;
    } else {
        /* Valid pointers */
    }

    ESP_LOGI(TAG, "ESP-NOW: Received %d bytes from %02X:%02X:%02X:%02X:%02X:%02X",
             len,
             recv_info->src_addr[0], recv_info->src_addr[1], recv_info->src_addr[2],
             recv_info->src_addr[3], recv_info->src_addr[4], recv_info->src_addr[5]);

    /* Verify it's from Node A */
    if (memcmp(recv_info->src_addr, s_node_a_mac, 6U) != 0) {
        ESP_LOGW(TAG, "ESP-NOW: Received from UNKNOWN MAC");
        return;
    } else {
        /* From Node A */
    }

    if ((size_t)len != sizeof(espnow_emergency_msg_t)) {
        ESP_LOGW(TAG, "ESP-NOW: Invalid message size %d (expected %zu)", len, sizeof(espnow_emergency_msg_t));
        return;
    } else {
        /* Valid size */
    }

    const espnow_emergency_msg_t *msg = (const espnow_emergency_msg_t *)data;

    if (msg->msg_type == 1U) {
        /* Emergency alert from Node A */
        ESP_LOGW(TAG, "========================================");
        ESP_LOGW(TAG, "ESP-NOW: EMERGENCY FROM NODE A!");
        ESP_LOGW(TAG, "  Reasons: 0x%02X", msg->emergency_reasons);
        ESP_LOGW(TAG, "  Posture: %s", msg->posture);
        ESP_LOGW(TAG, "  Value: %.1f", msg->sensor_value);
        ESP_LOGW(TAG, "  ACTIVATING BUZZER!");
        ESP_LOGW(TAG, "========================================");

        /* Turn on buzzer immediately */
        buzzer_on();

        /* Publish to MQTT */
        char alert_msg[128];
        (void)snprintf(alert_msg, sizeof(alert_msg),
                 "{\"source\":\"node_a\",\"reasons\":\"0x%02X\",\"posture\":\"%s\"}",
                 msg->emergency_reasons, msg->posture);
        mqtt_publish_security("security/espnow_alert", alert_msg);

        /* Keep buzzer on for 3 seconds, then turn off */
        vTaskDelay(pdMS_TO_TICKS(3000U));
        buzzer_off();
        ESP_LOGI(TAG, "Buzzer deactivated after emergency alert");
    } else {
        ESP_LOGI(TAG, "ESP-NOW: msg_type=%u (not emergency)", msg->msg_type);
    }
}

static void init_espnow(void)
{
    ESP_LOGI(TAG, "Initializing ESP-NOW...");

    /* Get current WiFi channel (set by AP connection) */
    uint8_t primary_channel = 0;
    wifi_second_chan_t secondary_channel = WIFI_SECOND_CHAN_NONE;
    ESP_ERROR_CHECK(esp_wifi_get_channel(&primary_channel, &secondary_channel));

    ESP_LOGI(TAG, "WiFi connected on channel %d - using for ESP-NOW", primary_channel);

    esp_err_t ret = esp_now_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ESP-NOW init failed: %s", esp_err_to_name(ret));
        return;
    } else {
        ESP_LOGI(TAG, "ESP-NOW initialized");
    }

    /* Register receive callback */
    ret = esp_now_register_recv_cb(espnow_recv_callback);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register recv callback: %s", esp_err_to_name(ret));
        return;
    } else {
        ESP_LOGI(TAG, "ESP-NOW receive callback registered");
    }

    /* Add Node A as peer */
    esp_now_peer_info_t peer_info = {
        .channel = primary_channel,  /* Use actual WiFi channel */
        .ifidx = WIFI_IF_STA,
        .encrypt = false,
    };
    (void)memcpy(peer_info.peer_addr, s_node_a_mac, 6U);

    ret = esp_now_add_peer(&peer_info);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add peer: %s", esp_err_to_name(ret));
        return;
    } else {
        /* Peer added */
    }

    ESP_LOGI(TAG, "ESP-NOW READY on channel %d - Listening for emergencies", primary_channel);
    ESP_LOGI(TAG, "  Expecting MAC: %02X:%02X:%02X:%02X:%02X:%02X",
             s_node_a_mac[0], s_node_a_mac[1], s_node_a_mac[2],
             s_node_a_mac[3], s_node_a_mac[4], s_node_a_mac[5]);
}

/* ========== MQTT Task ========== */

void mqtt_task_b(void *pvParameters)
{
    (void)pvParameters;  /* MISRA Rule 2.7 */

    ESP_LOGI(TAG, "MQTT task (Node B) started");

    /* Initialize WiFi */
    init_wifi();
    ESP_LOGI(TAG, "Waiting for WiFi...");
    vTaskDelay(pdMS_TO_TICKS(3000U));

    /* Initialize SNTP (needed for TLS) */
#if USE_HIVEMQ_CLOUD
    init_sntp();
#endif

    /* Initialize MQTT */
    init_mqtt();
    ESP_LOGI(TAG, "Waiting for MQTT...");
    vTaskDelay(pdMS_TO_TICKS(2000U));

    /* Initialize ESP-NOW after WiFi is up */
    init_espnow();

    /* Publish online status */
    mqtt_publish_security("security/status", "ONLINE");

    /* Task waits - callbacks handle ESP-NOW reception */
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000U));
    }

    vTaskDelete(NULL);
}
