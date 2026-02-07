#ifndef VITALS_MONITOR_TASK_H
#define VITALS_MONITOR_TASK_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* Vitals Monitor Task Function Prototype (MISRA Rule 8.4) */
void vitals_monitor_task(void* pvParameters);

#endif /* VITALS_MONITOR_TASK_H */
