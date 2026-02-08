#ifndef MPU6050_H
#define MPU6050_H

#include "esp_err.h"
#include "utils/data_structures.h"

/* MPU6050 Sensor Function Prototypes (MISRA Rule 8.4) */
esp_err_t mpu6050_init(void);
esp_err_t mpu6050_read(mpu6050_data_t *data);

#endif /* MPU6050_H */
