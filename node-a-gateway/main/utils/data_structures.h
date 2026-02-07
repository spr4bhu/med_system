#ifndef DATA_STRUCTURES_H
#define DATA_STRUCTURES_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"

/* Global FreeRTOS objects (defined in main.c) */
extern QueueHandle_t sensor_data_queue;
extern QueueHandle_t node_b_data_queue;
extern SemaphoreHandle_t i2c_mutex;
extern EventGroupHandle_t emergency_event_group;

/* Event group bits */
#define FALL_DETECTED_BIT    (1 << 0)
#define VITALS_ABNORMAL_BIT  (1 << 1)
#define SOS_PRESSED_BIT      (1 << 2)

#endif /* DATA_STRUCTURES_H */
