#ifndef ACCESS_CONTROL_TASK_H
#define ACCESS_CONTROL_TASK_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* Access Control Task Function Prototype (MISRA Rule 8.4) */
void access_control_task(void* pvParameters);

#endif /* ACCESS_CONTROL_TASK_H */
