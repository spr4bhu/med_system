#ifndef FALL_DETECTION_TASK_H
#define FALL_DETECTION_TASK_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* Fall Detection Task Function Prototype (MISRA Rule 8.4) */
void fall_detection_task(void* pvParameters);

#endif /* FALL_DETECTION_TASK_H */
