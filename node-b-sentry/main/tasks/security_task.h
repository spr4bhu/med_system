#ifndef SECURITY_TASK_H
#define SECURITY_TASK_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* Security Task Function Prototype (MISRA Rule 8.4) */
void security_task(void *pvParameters);

#endif /* SECURITY_TASK_H */
