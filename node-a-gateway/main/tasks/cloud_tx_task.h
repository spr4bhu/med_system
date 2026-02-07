#ifndef CLOUD_TX_TASK_H
#define CLOUD_TX_TASK_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* Cloud TX Task Function Prototype (MISRA Rule 8.4) */
void cloud_tx_task(void* pvParameters);

#endif /* CLOUD_TX_TASK_H */
