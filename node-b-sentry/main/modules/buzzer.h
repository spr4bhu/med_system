#ifndef BUZZER_H
#define BUZZER_H

#include "esp_err.h"

/* Buzzer Function Prototypes (MISRA Rule 8.4) */
esp_err_t buzzer_init(void);
void buzzer_on(void);
void buzzer_off(void);

#endif /* BUZZER_H */
