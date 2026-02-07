#ifndef RFID_RC522_H
#define RFID_RC522_H

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

/* RFID RC522 Function Prototypes (MISRA Rule 8.4) */
esp_err_t rfid_rc522_init(void);
esp_err_t rfid_rc522_read_uid(uint32_t* uid);
bool rfid_rc522_is_authorized(uint32_t uid);

#endif /* RFID_RC522_H */
