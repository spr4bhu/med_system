#ifndef MQTT_TASK_H
#define MQTT_TASK_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* MQTT Task Function Prototype (MISRA Rule 8.4) */
void mqtt_task_b(void *pvParameters);

/* MQTT Publish Function (called from other tasks) */
void mqtt_publish_security(const char *topic, const char *data);

#endif /* MQTT_TASK_H */
