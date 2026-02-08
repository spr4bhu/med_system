#ifndef HW827_H
#define HW827_H

#include "esp_err.h"
#include "utils/data_structures.h"

/* HW-827 Heart Rate Sensor Function Prototypes (MISRA Rule 8.4) */
esp_err_t hw827_init(void);
esp_err_t hw827_read(hw827_data_t *data);
uint8_t hw827_detect_peak(const hw827_data_t *current, const hw827_data_t *previous);

#endif /* HW827_H */
