#ifndef IR_SENSOR_H
#define IR_SENSOR_H

#include "esp_err.h"
#include <stdbool.h>

/* IR Sensor Function Prototypes (MISRA Rule 8.4) */
esp_err_t ir_sensor_init(void);
bool ir_sensor_is_detected(void);

#endif /* IR_SENSOR_H */
