#ifndef GATEWAY_RX_TASK_H
#define GATEWAY_RX_TASK_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* Gateway RX Task Function Prototype (MISRA Rule 8.4) */
void gateway_rx_task(void* pvParameters);

#endif /* GATEWAY_RX_TASK_H */
