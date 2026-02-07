#ifndef DHT22_H
#define DHT22_H

#include "esp_err.h"
#include <stdint.h>

/* DHT22 Function Prototypes (MISRA Rule 8.4) */
esp_err_t dht22_init(void);
esp_err_t dht22_read_temp(float* temperature, float* humidity);

#endif /* DHT22_H */
