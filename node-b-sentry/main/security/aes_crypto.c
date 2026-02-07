#include "aes_crypto.h"
#include "encryption_keys.h"
#include "protocol.h"
#include "mbedtls/aes.h"
#include "esp_log.h"
#include "esp_random.h"
#include <string.h>

static const char* TAG = "AES_CRYPTO";

esp_err_t aes_encrypt(const uint8_t* plaintext, size_t len,
                      uint8_t* ciphertext, uint8_t* iv) {
    if ((plaintext == NULL) || (ciphertext == NULL) || (iv == NULL)) {
        ESP_LOGE(TAG, "NULL pointer passed to aes_encrypt");
        return ESP_ERR_INVALID_ARG;
    } else {
        /* Valid pointers */
    }

    if (len == 0U) {
        ESP_LOGE(TAG, "Invalid length: 0");
        return ESP_ERR_INVALID_ARG;
    } else {
        /* Valid length */
    }

    /* Generate random IV */
    esp_fill_random(iv, AES_IV_SIZE);

    /* Initialize AES context */
    mbedtls_aes_context aes_ctx;
    mbedtls_aes_init(&aes_ctx);

    /* Set encryption key */
    int ret = mbedtls_aes_setkey_enc(&aes_ctx, AES_KEY, AES_KEY_SIZE * 8U);
    if (ret != 0) {
        ESP_LOGE(TAG, "AES setkey_enc failed: %d", ret);
        mbedtls_aes_free(&aes_ctx);
        return ESP_FAIL;
    } else {
        /* Key set successfully */
    }

    /* Create IV copy (mbedTLS modifies it) */
    uint8_t iv_copy[AES_IV_SIZE];
    (void)memcpy(iv_copy, iv, AES_IV_SIZE);

    /* Perform AES-128-CBC encryption */
    ret = mbedtls_aes_crypt_cbc(
        &aes_ctx,
        MBEDTLS_AES_ENCRYPT,
        len,
        iv_copy,
        plaintext,
        ciphertext
    );

    mbedtls_aes_free(&aes_ctx);

    if (ret != 0) {
        ESP_LOGE(TAG, "AES encryption failed: %d", ret);
        return ESP_FAIL;
    } else {
        ESP_LOGD(TAG, "AES encryption successful");
    }

    return ESP_OK;
}

esp_err_t aes_decrypt(const uint8_t* ciphertext, size_t len,
                      uint8_t* plaintext, const uint8_t* iv) {
    if ((ciphertext == NULL) || (plaintext == NULL) || (iv == NULL)) {
        ESP_LOGE(TAG, "NULL pointer passed to aes_decrypt");
        return ESP_ERR_INVALID_ARG;
    } else {
        /* Valid pointers */
    }

    if (len == 0U) {
        ESP_LOGE(TAG, "Invalid length: 0");
        return ESP_ERR_INVALID_ARG;
    } else {
        /* Valid length */
    }

    /* Initialize AES context */
    mbedtls_aes_context aes_ctx;
    mbedtls_aes_init(&aes_ctx);

    /* Set decryption key */
    int ret = mbedtls_aes_setkey_dec(&aes_ctx, AES_KEY, AES_KEY_SIZE * 8U);
    if (ret != 0) {
        ESP_LOGE(TAG, "AES setkey_dec failed: %d", ret);
        mbedtls_aes_free(&aes_ctx);
        return ESP_FAIL;
    } else {
        /* Key set successfully */
    }

    /* Create IV copy (mbedTLS modifies it) */
    uint8_t iv_copy[AES_IV_SIZE];
    (void)memcpy(iv_copy, iv, AES_IV_SIZE);

    /* Perform AES-128-CBC decryption */
    ret = mbedtls_aes_crypt_cbc(
        &aes_ctx,
        MBEDTLS_AES_DECRYPT,
        len,
        iv_copy,
        ciphertext,
        plaintext
    );

    mbedtls_aes_free(&aes_ctx);

    if (ret != 0) {
        ESP_LOGE(TAG, "AES decryption failed: %d", ret);
        return ESP_FAIL;
    } else {
        ESP_LOGD(TAG, "AES decryption successful");
    }

    return ESP_OK;
}
