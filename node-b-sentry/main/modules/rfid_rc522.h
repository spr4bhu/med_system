#ifndef RFID_RC522_H
#define RFID_RC522_H

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

/* RFID verification result */
typedef struct {
    bool detected;        /* Was a card detected? */
    bool authorized;      /* Is the card authorized? */
    const char *name;     /* Tag name/description */
} rfid_result_t;

/* RFID RC522 Function Prototypes (MISRA Rule 8.4) */
esp_err_t rfid_rc522_init(void);
rfid_result_t rfid_rc522_check_card(void);

#endif /* RFID_RC522_H */
