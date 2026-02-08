#ifndef EMERGENCY_BUTTON_H
#define EMERGENCY_BUTTON_H

#include "esp_err.h"
#include <stdbool.h>

/* Emergency Button Function Prototypes (MISRA Rule 8.4) */
esp_err_t emergency_button_init(void);
bool is_emergency_button_pressed(void);

#endif /* EMERGENCY_BUTTON_H */
