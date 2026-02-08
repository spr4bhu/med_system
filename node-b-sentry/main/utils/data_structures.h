#ifndef DATA_STRUCTURES_H
#define DATA_STRUCTURES_H

#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"

/* ========== Global FreeRTOS objects (defined in main.c) ========== */
extern SemaphoreHandle_t spi_mutex;
extern EventGroupHandle_t alarm_event_group;

/* ========== Event group bits ========== */
#define EMERGENCY_FROM_NODE_A_BIT  (1 << 0)

#endif /* DATA_STRUCTURES_H */
