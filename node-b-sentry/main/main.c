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
#include "modules/rfid_rc522.h"
#include "modules/ir_sensor.h"
#include "modules/buzzer.h"
#include "tasks/access_control_task.h"
#include "tasks/intrusion_detect_task.h"
#include "tasks/espnow_tx_task.h"

static const char* TAG = "NODE_B";

/* Global FreeRTOS objects (defined here, declared in data_structures.h) */
QueueHandle_t event_queue;
SemaphoreHandle_t spi_mutex;
EventGroupHandle_t alarm_event_group;

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
    ESP_LOGI(TAG, "*    Node B Sentry - Medical Monitor System     *");
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

    /* Initialize WiFi (for ESP-NOW only) */
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ret = esp_wifi_init(&cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "WiFi init failed: %d", ret);
        return;
    } else {
        /* WiFi init OK */
    }

    ret = esp_wifi_set_mode(WIFI_MODE_STA);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "WiFi set mode failed: %d", ret);
        return;
    } else {
        /* Mode set */
    }

    ret = esp_wifi_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "WiFi start failed: %d", ret);
        return;
    } else {
        ESP_LOGI(TAG, "WiFi started for ESP-NOW");
    }

    /* Create FreeRTOS synchronization objects */
    ESP_LOGI(TAG, "Creating FreeRTOS objects...");

    event_queue = xQueueCreate(20, sizeof(espnow_packet_t));
    if (event_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create event_queue");
        return;
    } else {
        /* Queue created */
    }

    spi_mutex = xSemaphoreCreateMutex();
    if (spi_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create spi_mutex");
        return;
    } else {
        /* Mutex created */
    }

    alarm_event_group = xEventGroupCreate();
    if (alarm_event_group == NULL) {
        ESP_LOGE(TAG, "Failed to create alarm_event_group");
        return;
    } else {
        ESP_LOGI(TAG, "FreeRTOS objects created successfully");
    }

    /* Initialize hardware modules */
    ESP_LOGI(TAG, "Initializing hardware modules...");

    ret = rfid_rc522_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "RFID RC522 init failed: %d", ret);
        /* Continue anyway */
    } else {
        ESP_LOGI(TAG, "RFID RC522 initialized");
    }

    ret = ir_sensor_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "IR sensor init failed: %d", ret);
        /* Continue anyway */
    } else {
        ESP_LOGI(TAG, "IR sensor initialized");
    }

    ret = buzzer_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Buzzer init failed: %d", ret);
        /* Continue anyway */
    } else {
        ESP_LOGI(TAG, "Buzzer initialized");
    }

    /* Create FreeRTOS tasks */
    ESP_LOGI(TAG, "Creating tasks...");

    BaseType_t task_ret = pdFAIL;

    task_ret = xTaskCreate(
        access_control_task,
        "access_ctrl",
        ACCESS_TASK_STACK_SIZE,
        NULL,
        ACCESS_CTRL_TASK_PRIORITY,
        NULL
    );
    if (task_ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create access_control_task");
        return;
    } else {
        ESP_LOGI(TAG, "Access control task created");
    }

    task_ret = xTaskCreate(
        intrusion_detect_task,
        "intrusion",
        INTRUSION_TASK_STACK_SIZE,
        NULL,
        INTRUSION_TASK_PRIORITY,
        NULL
    );
    if (task_ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create intrusion_detect_task");
        return;
    } else {
        ESP_LOGI(TAG, "Intrusion detection task created");
    }

    task_ret = xTaskCreate(
        espnow_tx_task,
        "espnow_tx",
        ESPNOW_TX_TASK_STACK_SIZE,
        NULL,
        ESPNOW_TX_TASK_PRIORITY,
        NULL
    );
    if (task_ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create espnow_tx_task");
        return;
    } else {
        ESP_LOGI(TAG, "ESP-NOW TX task created");
    }

    ESP_LOGI(TAG, "**************************************************");
    ESP_LOGI(TAG, "*  All systems initialized - Node B running      *");
    ESP_LOGI(TAG, "**************************************************");

    /* app_main returns here, FreeRTOS scheduler continues running tasks */
}
