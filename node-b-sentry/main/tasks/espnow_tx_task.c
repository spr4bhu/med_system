#include "espnow_tx_task.h"
#include "config.h"
#include "utils/data_structures.h"
#include "security/aes_crypto.h"
#include "message_types.h"
#include "protocol.h"
#include "encryption_keys.h"
#include "esp_now.h"
#include "esp_log.h"
#include <string.h>

static const char* TAG = "ESPNOW_TX";

/* CRC16 function prototype */
extern uint16_t crc16_calculate(const uint8_t* data, size_t len);

/* ESP-NOW send callback */
static void espnow_send_cb(const uint8_t* mac_addr, esp_now_send_status_t status) {
    if (mac_addr == NULL) {
        ESP_LOGE(TAG, "NULL MAC address in send callback");
        return;
    } else {
        /* Valid pointer */
    }

    if (status == ESP_NOW_SEND_SUCCESS) {
        ESP_LOGI(TAG, "ESP-NOW send successful");
    } else {
        ESP_LOGW(TAG, "ESP-NOW send failed");
    }
}

void espnow_tx_task(void* pvParameters) {
    (void)pvParameters;  /* Unused parameter (MISRA Rule 2.7) */

    ESP_LOGI(TAG, "ESP-NOW TX task started");

    /* Initialize ESP-NOW */
    esp_err_t ret = esp_now_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ESP-NOW init failed: %d", ret);
        vTaskDelete(NULL);
        return;
    } else {
        ESP_LOGI(TAG, "ESP-NOW initialized");
    }

    /* Register send callback */
    ret = esp_now_register_send_cb(espnow_send_cb);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ESP-NOW register send callback failed: %d", ret);
        (void)esp_now_deinit();
        vTaskDelete(NULL);
        return;
    } else {
        /* Callback registered */
    }

    /* Add Node A as peer */
    esp_now_peer_info_t peer_info;
    (void)memset(&peer_info, 0, sizeof(peer_info));
    (void)memcpy(peer_info.peer_addr, NODE_A_MAC, 6);
    peer_info.channel = ESPNOW_CHANNEL;
    peer_info.ifidx = WIFI_IF_STA;
    peer_info.encrypt = false;  /* We handle encryption ourselves */

    ret = esp_now_add_peer(&peer_info);
    if ((ret != ESP_OK) && (ret != ESP_ERR_ESPNOW_EXIST)) {
        ESP_LOGE(TAG, "ESP-NOW add peer failed: %d", ret);
        (void)esp_now_deinit();
        vTaskDelete(NULL);
        return;
    } else {
        ESP_LOGI(TAG, "Node A added as ESP-NOW peer");
    }

    /* Main loop - wait for events and send to Node A */
    while (1) {
        /* Check if any alarm bits are set */
        EventBits_t events = xEventGroupGetBits(alarm_event_group);

        if ((events & (INTRUSION_BIT | UNAUTHORIZED_ACCESS_BIT)) != 0U) {
            /* Create and send packet */
            espnow_packet_t packet;
            (void)memset(&packet, 0, sizeof(packet));

            packet.node_id = NODE_ID_B;

            if ((events & INTRUSION_BIT) != 0U) {
                packet.msg_type = MSG_TYPE_INTRUSION;
                ESP_LOGI(TAG, "Preparing intrusion packet");
            } else {
                packet.msg_type = MSG_TYPE_ACCESS_LOG;
                ESP_LOGI(TAG, "Preparing access log packet");
            }

            /* Encrypt payload (simplified for hackathon) */
            uint8_t plaintext[128];
            (void)memset(plaintext, 0, sizeof(plaintext));

            ret = aes_encrypt(
                plaintext,
                sizeof(plaintext),
                packet.encrypted_payload,
                packet.iv
            );

            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "Encryption failed: %d", ret);
            } else {
                /* Calculate CRC */
                packet.timestamp_ms = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
                packet.crc16 = crc16_calculate(
                    (const uint8_t*)&packet,
                    sizeof(espnow_packet_t) - sizeof(uint16_t)
                );

                /* Send via ESP-NOW */
                ret = esp_now_send(NODE_A_MAC, (const uint8_t*)&packet, sizeof(packet));

                if (ret == ESP_OK) {
                    ESP_LOGI(TAG, "ESP-NOW packet sent successfully");
                    /* Clear event bits after successful send */
                    (void)xEventGroupClearBits(alarm_event_group, events);
                } else {
                    ESP_LOGW(TAG, "ESP-NOW send failed: %d", ret);
                }
            }
        } else {
            /* No events to send */
        }

        /* Sleep for transmission interval */
        vTaskDelay(pdMS_TO_TICKS(ESPNOW_TX_INTERVAL_MS));
    }

    /* Cleanup (never reached) */
    (void)esp_now_unregister_send_cb();
    (void)esp_now_deinit();
    vTaskDelete(NULL);
}
