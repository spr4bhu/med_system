#ifndef DATA_STRUCTURES_H
#define DATA_STRUCTURES_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"

/* Global FreeRTOS objects (defined in main.c) */
extern QueueHandle_t event_queue;
extern SemaphoreHandle_t spi_mutex;
extern EventGroupHandle_t alarm_event_group;

/* Event group bits */
#define INTRUSION_BIT           (1 << 0)
#define UNAUTHORIZED_ACCESS_BIT (1 << 1)

#endif /* DATA_STRUCTURES_H */
