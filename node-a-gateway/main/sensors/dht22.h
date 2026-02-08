#ifndef DHT11_H
#define DHT11_H

#include "esp_err.h"
#include "utils/data_structures.h"

/* DHT11 Sensor Function Prototypes (MISRA Rule 8.4) */
esp_err_t dht11_init(void);
int dht11_read(dht11_data_t *data);

#endif /* DHT11_H */
