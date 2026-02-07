#ifndef ESPNOW_TX_TASK_H
#define ESPNOW_TX_TASK_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* ESP-NOW TX Task Function Prototype (MISRA Rule 8.4) */
void espnow_tx_task(void* pvParameters);

#endif /* ESPNOW_TX_TASK_H */
