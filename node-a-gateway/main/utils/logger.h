#ifndef LOGGER_H
#define LOGGER_H

#include "esp_log.h"

/* Use ESP-IDF logging macros */
#define LOG_TAG "NODE_A"
#define LOG_INFO(fmt, ...)  ESP_LOGI(LOG_TAG, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) ESP_LOGE(LOG_TAG, fmt, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...) ESP_LOGD(LOG_TAG, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  ESP_LOGW(LOG_TAG, fmt, ##__VA_ARGS__)

#endif /* LOGGER_H */
