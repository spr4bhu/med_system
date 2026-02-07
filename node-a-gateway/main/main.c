#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"

#include "config.h"
#include "sensors/mpu6050.h"
#include "sensors/max30102.h"
#include "sensors/dht22.h"
#include "sensors/emergency_button.h"
#include "tasks/sensor_task.h"
#include "tasks/fall_detection_task.h"
#include "tasks/vitals_monitor_task.h"
#include "tasks/gateway_rx_task.h"
#include "tasks/cloud_tx_task.h"

static const char* TAG = "NODE_A";

/* Global FreeRTOS objects (defined here, declared in data_structures.h) */
QueueHandle_t sensor_data_queue;
QueueHandle_t node_b_data_queue;
SemaphoreHandle_t i2c_mutex;
EventGroupHandle_t emergency_event_group;

/* Function prototypes (MISRA Rule 8.4) */
static void print_mac_address(void);

static void print_mac_address(void) {
    uint8_t mac[6];
    esp_err_t ret = esp_efuse_mac_get_default(mac);

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Base MAC Address: %02X:%02X:%02X:%02X:%02X:%02X",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    } else {
        ESP_LOGE(TAG, "Failed to get MAC address: %d", ret);
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "**************************************************");
    ESP_LOGI(TAG, "*    Node A Gateway - Medical Monitor System    *");
    ESP_LOGI(TAG, "**************************************************");

    /* Print MAC address for encryption_keys.h */
    print_mac_address();

    /* Initialize NVS (MISRA Rule 17.7 - check return value) */
    esp_err_t ret = nvs_flash_init();
    if ((ret == ESP_ERR_NVS_NO_FREE_PAGES) || (ret == ESP_ERR_NVS_NEW_VERSION_FOUND)) {
        ESP_LOGW(TAG, "NVS partition empty, erasing...");
        ret = nvs_flash_erase();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "NVS erase failed: %d", ret);
            return;
        } else {
            /* Retry init */
            ret = nvs_flash_init();
        }
    } else {
        /* NVS init OK or other error */
    }

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "NVS init failed: %d", ret);
        return;
    } else {
        ESP_LOGI(TAG, "NVS initialized");
    }

    /* Initialize network interface */
    ret = esp_netif_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Netif init failed: %d", ret);
        return;
    } else {
        /* Netif init OK */
    }

    /* Create default event loop */
    ret = esp_event_loop_create_default();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Event loop create failed: %d", ret);
        return;
    } else {
        /* Event loop created */
    }

    /* Create default WiFi station */
    esp_netif_t* sta_netif = esp_netif_create_default_wifi_sta();
    if (sta_netif == NULL) {
        ESP_LOGE(TAG, "Failed to create WiFi station interface");
        return;
    } else {
        ESP_LOGI(TAG, "WiFi station interface created");
    }

    /* Create FreeRTOS synchronization objects */
    ESP_LOGI(TAG, "Creating FreeRTOS objects...");

    sensor_data_queue = xQueueCreate(10, sizeof(sensor_data_t));
    if (sensor_data_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create sensor_data_queue");
        return;
    } else {
        /* Queue created */
    }

    node_b_data_queue = xQueueCreate(20, sizeof(espnow_packet_t));
    if (node_b_data_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create node_b_data_queue");
        return;
    } else {
        /* Queue created */
    }

    i2c_mutex = xSemaphoreCreateMutex();
    if (i2c_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create i2c_mutex");
        return;
    } else {
        /* Mutex created */
    }

    emergency_event_group = xEventGroupCreate();
    if (emergency_event_group == NULL) {
        ESP_LOGE(TAG, "Failed to create emergency_event_group");
        return;
    } else {
        ESP_LOGI(TAG, "FreeRTOS objects created successfully");
    }

    /* Initialize sensors */
    ESP_LOGI(TAG, "Initializing sensors...");

    ret = mpu6050_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "MPU6050 init failed: %d", ret);
        /* Continue anyway */
    } else {
        ESP_LOGI(TAG, "MPU6050 initialized");
    }

    ret = max30102_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "MAX30102 init failed: %d", ret);
        /* Continue anyway */
    } else {
        ESP_LOGI(TAG, "MAX30102 initialized");
    }

    ret = dht22_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "DHT22 init failed: %d", ret);
        /* Continue anyway */
    } else {
        ESP_LOGI(TAG, "DHT22 initialized");
    }

    ret = emergency_button_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Emergency button init failed: %d", ret);
        /* Continue anyway */
    } else {
        ESP_LOGI(TAG, "Emergency button initialized");
    }

    /* Create FreeRTOS tasks */
    ESP_LOGI(TAG, "Creating tasks...");

    BaseType_t task_ret = pdFAIL;

    task_ret = xTaskCreate(
        sensor_task,
        "sensor",
        SENSOR_TASK_STACK_SIZE,
        NULL,
        SENSOR_TASK_PRIORITY,
        NULL
    );
    if (task_ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create sensor_task");
        return;
    } else {
        ESP_LOGI(TAG, "Sensor task created");
    }

    task_ret = xTaskCreate(
        fall_detection_task,
        "fall_detect",
        FALL_TASK_STACK_SIZE,
        NULL,
        FALL_DETECT_TASK_PRIORITY,
        NULL
    );
    if (task_ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create fall_detection_task");
        return;
    } else {
        ESP_LOGI(TAG, "Fall detection task created");
    }

    task_ret = xTaskCreate(
        vitals_monitor_task,
        "vitals",
        VITALS_TASK_STACK_SIZE,
        NULL,
        VITALS_TASK_PRIORITY,
        NULL
    );
    if (task_ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create vitals_monitor_task");
        return;
    } else {
        ESP_LOGI(TAG, "Vitals monitor task created");
    }

    task_ret = xTaskCreate(
        gateway_rx_task,
        "gateway_rx",
        GATEWAY_TASK_STACK_SIZE,
        NULL,
        GATEWAY_RX_TASK_PRIORITY,
        NULL
    );
    if (task_ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create gateway_rx_task");
        return;
    } else {
        ESP_LOGI(TAG, "Gateway RX task created");
    }

    task_ret = xTaskCreate(
        cloud_tx_task,
        "cloud_tx",
        CLOUD_TX_TASK_STACK_SIZE,
        NULL,
        CLOUD_TX_TASK_PRIORITY,
        NULL
    );
    if (task_ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create cloud_tx_task");
        return;
    } else {
        ESP_LOGI(TAG, "Cloud TX task created");
    }

    ESP_LOGI(TAG, "**************************************************");
    ESP_LOGI(TAG, "*  All systems initialized - Node A running      *");
    ESP_LOGI(TAG, "**************************************************");

    /* app_main returns here, FreeRTOS scheduler continues running tasks */
}
