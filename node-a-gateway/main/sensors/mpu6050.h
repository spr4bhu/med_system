#ifndef MPU6050_H
#define MPU6050_H

#include "esp_err.h"
#include <stdint.h>

/* MPU6050 Function Prototypes (MISRA Rule 8.4) */
esp_err_t mpu6050_init(void);
esp_err_t mpu6050_read_accel(float* ax, float* ay, float* az);
esp_err_t mpu6050_read_gyro(float* gx, float* gy, float* gz);

#endif /* MPU6050_H */
