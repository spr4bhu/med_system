#ifndef DHT22_H
#define DHT22_H

#include "esp_err.h"
#include <stdint.h>

/* DHT11 Temperature Sensor Function Prototypes (MISRA Rule 8.4) */
/* Note: Using DHT22 filename for compatibility, but sensor is DHT11 */
esp_err_t dht22_init(void);
esp_err_t dht22_read_temp(float* temperature, float* humidity);

#endif /* DHT22_H */
