#include "gateway_rx_task.h"
#include "config.h"
#include "utils/data_structures.h"
#include "security/aes_crypto.h"
#include "message_types.h"
#include "protocol.h"
#include "esp_now.h"
#include "esp_log.h"
#include <string.h>

static const char* TAG = "GATEWAY_RX";

/* CRC16 function prototype */
extern uint16_t crc16_calculate(const uint8_t* data, size_t len);

/* ESP-NOW receive callback (MISRA Rule 8.4) */
static void espnow_recv_cb(const esp_now_recv_info_t* recv_info, const uint8_t* data, int len) {
    if ((recv_info == NULL) || (data == NULL)) {
        ESP_LOGE(TAG, "NULL pointer in ESP-NOW callback");
        return;
    } else {
        /* Valid pointers */
    }

    /* Type-safe size check (MISRA 10.x): reject negative len, then compare as size_t */
    if (len < 0) {
        ESP_LOGW(TAG, "Invalid packet length: negative");
        return;
    } else if ((size_t)len != sizeof(espnow_packet_t)) {
        ESP_LOGW(TAG, "Invalid packet size: %d (expected %zu)", len, sizeof(espnow_packet_t));
        return;
    } else {
        /* Valid size */
    }

    /* Copy packet */
    espnow_packet_t packet;
    (void)memcpy(&packet, data, sizeof(espnow_packet_t));

    /* Verify CRC (calculate over entire packet except CRC field) */
    uint16_t calc_crc = crc16_calculate(
        (const uint8_t*)&packet,
        sizeof(espnow_packet_t) - sizeof(uint16_t)
    );

    if (calc_crc != packet.crc16) {
        ESP_LOGW(TAG, "CRC mismatch: calc=0x%04X, recv=0x%04X", calc_crc, packet.crc16);
        return;
    } else {
        ESP_LOGI(TAG, "ESP-NOW packet received from Node B, CRC OK");
    }

    /* Decrypt payload */
    uint8_t plaintext[128];
    esp_err_t ret = aes_decrypt(
        packet.encrypted_payload,
        sizeof(packet.encrypted_payload),
        plaintext,
        packet.iv
    );

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Decryption failed: %d", ret);
        return;
    } else {
        /* Decryption successful */
    }

    /* Send to queue for cloud transmission */
    BaseType_t queue_ret = xQueueSend(
        node_b_data_queue,
        &packet,
        pdMS_TO_TICKS(100U)
    );

    if (queue_ret != pdPASS) {
        ESP_LOGW(TAG, "Failed to queue Node B data");
    } else {
        ESP_LOGD(TAG, "Node B data queued for cloud transmission");
    }
}

void gateway_rx_task(void* pvParameters) {
    (void)pvParameters;  /* Unused parameter (MISRA Rule 2.7) */

    ESP_LOGI(TAG, "Gateway RX task started");

    /* Initialize ESP-NOW */
    esp_err_t ret = esp_now_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ESP-NOW init failed: %d", ret);
        vTaskDelete(NULL);
        return;
    } else {
        ESP_LOGI(TAG, "ESP-NOW initialized");
    }

    /* Register receive callback */
    ret = esp_now_register_recv_cb(espnow_recv_cb);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ESP-NOW register recv callback failed: %d", ret);
        (void)esp_now_deinit();
        vTaskDelete(NULL);
        return;
    } else {
        ESP_LOGI(TAG, "ESP-NOW receive callback registered");
    }

    /* Task just waits - callbacks handle reception */
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000U));
        ESP_LOGD(TAG, "Gateway RX task running");
    }

    /* Never reached; task runs until shutdown (MISRA Rule 2.1 - no unreachable code) */
}
