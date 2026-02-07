#ifndef BUZZER_H
#define BUZZER_H

#include "esp_err.h"
#include <stdint.h>

/* Buzzer Function Prototypes (MISRA Rule 8.4) */
esp_err_t buzzer_init(void);
esp_err_t buzzer_alarm_on(uint32_t frequency, uint32_t duration_ms);
esp_err_t buzzer_alarm_off(void);

#endif /* BUZZER_H */
