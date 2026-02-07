#ifndef INTRUSION_DETECT_TASK_H
#define INTRUSION_DETECT_TASK_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* Intrusion Detection Task Function Prototype (MISRA Rule 8.4) */
void intrusion_detect_task(void* pvParameters);

#endif /* INTRUSION_DETECT_TASK_H */
