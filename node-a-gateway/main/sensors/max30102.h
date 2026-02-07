#ifndef MAX30102_H
#define MAX30102_H

#include "esp_err.h"
#include <stdint.h>

/* MAX30102 Function Prototypes (MISRA Rule 8.4) */
esp_err_t max30102_init(void);
esp_err_t max30102_read_bpm(float* heart_rate);
esp_err_t max30102_read_spo2(float* spo2);

#endif /* MAX30102_H */
