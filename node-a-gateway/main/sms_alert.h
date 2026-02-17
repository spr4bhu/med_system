/*
 * SMS Alert via Twilio or IFTTT Webhook
 * MISRA-C compliant version
 */

#ifndef SMS_ALERT_H
#define SMS_ALERT_H

#include "config.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <stdio.h>

/* Rate limiting - only send SMS once per emergency type every 60 seconds */
#define SMS_COOLDOWN_MS 60000U

/* ========== IFTTT Webhook ========== */

#if USE_IFTTT
static esp_err_t send_sms_ifttt(const char *value1, const char *value2, const char *value3)
{
    static const char *TAG_IFTTT = "SMS_IFTTT";

    /* Build IFTTT Webhook URL */
    char url[256];
    (void)snprintf(url, sizeof(url),
             "https://maker.ifttt.com/trigger/%s/with/key/%s",
             IFTTT_EVENT_NAME, IFTTT_KEY);

    /* Build JSON data */
    char json_data[512];
    (void)snprintf(json_data, sizeof(json_data),
             "{\"value1\":\"%s\",\"value2\":\"%s\",\"value3\":\"%s\"}",
             value1, value2, value3);

    /* HTTP client config */
    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 10000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG_IFTTT, "HTTP client init failed");
        return ESP_FAIL;
    } else {
        /* Client initialized */
    }

    (void)esp_http_client_set_header(client, "Content-Type", "application/json");
    (void)esp_http_client_set_post_field(client, json_data, (int)strlen(json_data));

    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        int status = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG_IFTTT, "IFTTT webhook sent! HTTP Status: %d", status);
    } else {
        ESP_LOGE(TAG_IFTTT, "IFTTT webhook failed: %s", esp_err_to_name(err));
    }

    (void)esp_http_client_cleanup(client);
    return err;
}
#endif

/* ========== Rate Limiting ========== */

static bool should_send_sms(const char *emergency_type)
{
    static char last_emergency_type[32] = {0};
    static uint32_t last_sms_time = 0U;
    uint32_t now = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);

    /* If different emergency type, allow immediately */
    if (strcmp(last_emergency_type, emergency_type) != 0) {
        (void)strncpy(last_emergency_type, emergency_type, sizeof(last_emergency_type) - 1U);
        last_emergency_type[sizeof(last_emergency_type) - 1U] = '\0';
        last_sms_time = now;
        return true;
    } else {
        /* Same type - check cooldown */
    }

    /* Same emergency type - check cooldown */
    if ((now - last_sms_time) >= SMS_COOLDOWN_MS) {
        last_sms_time = now;
        return true;
    } else {
        /* Still in cooldown */
    }

    return false;
}

/* ========== Unified Alert Function ========== */

static void send_emergency_sms(const char *emergency_type, const char *details)
{
    if (!should_send_sms(emergency_type)) {
        ESP_LOGW("SMS", "SMS rate limited (cooldown: 60s)");
        return;
    } else {
        /* Sending alert */
    }

    ESP_LOGW("SMS", "Sending email/SMS alert...");

#if USE_IFTTT
    (void)send_sms_ifttt(emergency_type, details, "ESP32_Patient_Monitor");
#else
    (void)emergency_type;
    (void)details;
    ESP_LOGW("SMS", "No SMS provider configured (set USE_IFTTT=1 in config.h)");
#endif
}

#endif /* SMS_ALERT_H */
