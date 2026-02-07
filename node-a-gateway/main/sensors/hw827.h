#ifndef HW827_H
#define HW827_H

#include "esp_err.h"
#include <stdint.h>

/* HW827 Heart Rate Sensor Function Prototypes (MISRA Rule 8.4) */
esp_err_t hw827_init(void);
esp_err_t hw827_read_bpm(float* heart_rate);

#endif /* HW827_H */
