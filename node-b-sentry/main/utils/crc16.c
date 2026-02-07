#include <stdint.h>
#include <stddef.h>

/* CRC16-CCITT implementation for packet validation */

uint16_t crc16_calculate(const uint8_t* data, size_t len) {
    uint16_t crc = 0xFFFFU;

    if (data == NULL) {
        return 0U;
    } else {
        /* Valid pointer - continue */
    }

    for (size_t i = 0U; i < len; i++) {
        crc ^= ((uint16_t)data[i] << 8);

        for (uint8_t bit = 0U; bit < 8U; bit++) {
            if ((crc & 0x8000U) != 0U) {
                crc = (crc << 1) ^ 0x1021U;
            } else {
                crc = crc << 1;
            }
        }
    }

    return crc;
}
