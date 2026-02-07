#ifndef AES_CRYPTO_H
#define AES_CRYPTO_H

#include "esp_err.h"
#include <stdint.h>
#include <stddef.h>

/* AES Encryption/Decryption Function Prototypes (MISRA Rule 8.4) */
esp_err_t aes_encrypt(const uint8_t* plaintext, size_t len,
                      uint8_t* ciphertext, uint8_t* iv);
esp_err_t aes_decrypt(const uint8_t* ciphertext, size_t len,
                      uint8_t* plaintext, const uint8_t* iv);

#endif /* AES_CRYPTO_H */
