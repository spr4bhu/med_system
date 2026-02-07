#ifndef SENSOR_TASK_H
#define SENSOR_TASK_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* Sensor Task Function Prototype (MISRA Rule 8.4) */
void sensor_task(void* pvParameters);

#endif /* SENSOR_TASK_H */
