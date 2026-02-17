/**
 * @file offline_logger.c
 * @brief Offline event logging implementation with SPIFFS storage
 */

#include "offline_logger.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "mqtt_client.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

static const char *TAG = "offline_logger";

/* SPIFFS configuration */
#define SPIFFS_BASE_PATH "/spiffs"
#define LOG_FILE_0 SPIFFS_BASE_PATH "/events_0.log"
#define LOG_FILE_1 SPIFFS_BASE_PATH "/events_1.log"
#define MAX_FILE_SIZE 32768  /* 32KB per file */

/* NVS configuration */
#define NVS_NAMESPACE "offline_log"
#define NVS_KEY_ACTIVE_FILE "active_file"
#define NVS_KEY_UPLOAD_CURSOR "upload_cursor"

/* External MQTT client (defined in cloud_tx_task.c) */
extern esp_mqtt_client_handle_t mqtt_client;

/**
 * @brief Internal state structure
 */
typedef struct {
    bool initialized;           /**< Initialization flag */
    uint8_t active_file_index;  /**< 0 or 1 (which log file is active) */
    uint32_t upload_cursor;     /**< Byte offset for next upload */
    SemaphoreHandle_t mutex;    /**< Thread safety for file operations */
} offline_logger_state_t;

static offline_logger_state_t s_state = {
    .initialized = false,
    .active_file_index = 0,
    .upload_cursor = 0,
    .mutex = NULL
};

/**
 * @brief Get the current active log file path
 */
static const char* get_active_log_file(void)
{
    return (s_state.active_file_index == 0) ? LOG_FILE_0 : LOG_FILE_1;
}

/**
 * @brief Get file size in bytes
 */
static long get_file_size(const char *filepath)
{
    struct stat st;
    long size = 0;

    if (stat(filepath, &st) == 0) {
        size = st.st_size;
    }

    return size;
}

/**
 * @brief Switch to the other log file (circular buffer)
 */
static esp_err_t switch_log_file(void)
{
    esp_err_t result = ESP_OK;
    nvs_handle_t nvs_handle = 0;

    /* Toggle active file index */
    s_state.active_file_index = (s_state.active_file_index == 0) ? 1 : 0;
    s_state.upload_cursor = 0;

    ESP_LOGI(TAG, "Switched to log file %d", s_state.active_file_index);

    /* Save new active file index to NVS */
    result = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (result == ESP_OK) {
        result = nvs_set_u8(nvs_handle, NVS_KEY_ACTIVE_FILE, s_state.active_file_index);
        if (result == ESP_OK) {
            result = nvs_commit(nvs_handle);
        } else {
            ESP_LOGE(TAG, "Failed to save active_file to NVS: %s", esp_err_to_name(result));
        }
        nvs_close(nvs_handle);
    } else {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(result));
    }

    /* Truncate the new active file (overwrite old data) */
    FILE *file = fopen(get_active_log_file(), "w");
    if (file != NULL) {
        fclose(file);
        ESP_LOGI(TAG, "Truncated %s", get_active_log_file());
    } else {
        ESP_LOGE(TAG, "Failed to truncate %s", get_active_log_file());
        result = ESP_FAIL;
    }

    return result;
}

esp_err_t offline_logger_init(void)
{
    esp_err_t result = ESP_OK;

    if (s_state.initialized) {
        ESP_LOGW(TAG, "Already initialized");
        return ESP_OK;
    }

    /* Create mutex for thread safety */
    s_state.mutex = xSemaphoreCreateMutex();
    if (s_state.mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create mutex");
        result = ESP_FAIL;
    } else {
        /* Configure SPIFFS */
        esp_vfs_spiffs_conf_t conf = {
            .base_path = SPIFFS_BASE_PATH,
            .partition_label = "storage",
            .max_files = 3,
            .format_if_mount_failed = true
        };

        /* Mount SPIFFS */
        result = esp_vfs_spiffs_register(&conf);
        if (result != ESP_OK) {
            if (result == ESP_FAIL) {
                ESP_LOGE(TAG, "Failed to mount SPIFFS");
            } else if (result == ESP_ERR_NOT_FOUND) {
                ESP_LOGE(TAG, "SPIFFS partition not found");
            } else {
                ESP_LOGE(TAG, "SPIFFS init failed: %s", esp_err_to_name(result));
            }
        } else {
            size_t total = 0;
            size_t used = 0;

            result = esp_spiffs_info("storage", &total, &used);
            if (result == ESP_OK) {
                ESP_LOGI(TAG, "SPIFFS mounted: %zu KB total, %zu KB used", total / 1024, used / 1024);
            } else {
                ESP_LOGW(TAG, "Failed to get SPIFFS info: %s", esp_err_to_name(result));
                /* Non-fatal - continue */
                result = ESP_OK;
            }

            /* Load metadata from NVS */
            nvs_handle_t nvs_handle = 0;
            esp_err_t nvs_result = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);

            if (nvs_result == ESP_OK) {
                /* Load active file index (default 0) */
                uint8_t active_file = 0;
                nvs_result = nvs_get_u8(nvs_handle, NVS_KEY_ACTIVE_FILE, &active_file);
                if (nvs_result == ESP_OK) {
                    s_state.active_file_index = active_file;
                } else {
                    s_state.active_file_index = 0;
                    (void)nvs_set_u8(nvs_handle, NVS_KEY_ACTIVE_FILE, 0);
                }

                /* Load upload cursor (default 0) */
                uint32_t cursor = 0;
                nvs_result = nvs_get_u32(nvs_handle, NVS_KEY_UPLOAD_CURSOR, &cursor);
                if (nvs_result == ESP_OK) {
                    s_state.upload_cursor = cursor;
                } else {
                    s_state.upload_cursor = 0;
                    (void)nvs_set_u32(nvs_handle, NVS_KEY_UPLOAD_CURSOR, 0);
                }

                (void)nvs_commit(nvs_handle);
                nvs_close(nvs_handle);

                ESP_LOGI(TAG, "Loaded metadata: active_file=%d, cursor=%lu",
                         s_state.active_file_index, s_state.upload_cursor);
            } else {
                ESP_LOGW(TAG, "Failed to open NVS namespace, using defaults");
                /* Non-fatal - use defaults */
            }

            s_state.initialized = true;
        }
    }

    return result;
}

esp_err_t offline_logger_log_event(const offline_event_t *event)
{
    esp_err_t result = ESP_OK;

    /* Validate input */
    if (event == NULL) {
        ESP_LOGE(TAG, "Cannot log NULL event");
        result = ESP_ERR_INVALID_ARG;
    } else if (!s_state.initialized) {
        ESP_LOGE(TAG, "Logger not initialized");
        result = ESP_ERR_INVALID_STATE;
    } else {
        /* Acquire mutex */
        BaseType_t mutex_taken = xSemaphoreTake(s_state.mutex, pdMS_TO_TICKS(1000));

        if (mutex_taken == pdTRUE) {
            /* Check if current file exceeds max size */
            long file_size = get_file_size(get_active_log_file());

            if (file_size >= MAX_FILE_SIZE) {
                ESP_LOGW(TAG, "Log file full (%ld bytes), switching files", file_size);
                result = switch_log_file();
            }

            if (result == ESP_OK) {
                /* Append event to active log file */
                FILE *file = fopen(get_active_log_file(), "a");

                if (file != NULL) {
                    /* Write CSV format: timestamp,type,posture,sensor_value,emergency_reasons */
                    int written = fprintf(file, "%lu,%u,%s,%.2f,0x%02X\n",
                                          event->timestamp,
                                          event->type,
                                          event->posture,
                                          event->sensor_value,
                                          event->emergency_reasons);

                    fclose(file);

                    if (written > 0) {
                        ESP_LOGD(TAG, "Logged event: type=%u, timestamp=%lu", event->type, event->timestamp);
                    } else {
                        ESP_LOGE(TAG, "Failed to write event to file");
                        result = ESP_FAIL;
                    }
                } else {
                    ESP_LOGE(TAG, "Failed to open %s for append", get_active_log_file());
                    result = ESP_FAIL;
                }
            }

            xSemaphoreGive(s_state.mutex);
        } else {
            ESP_LOGE(TAG, "Failed to acquire mutex");
            result = ESP_ERR_TIMEOUT;
        }
    }

    return result;
}

esp_err_t offline_logger_upload_pending(void)
{
    esp_err_t result = ESP_OK;

    ESP_LOGI(TAG, "Starting offline event upload");

    if (!s_state.initialized) {
        ESP_LOGE(TAG, "Logger not initialized");
        result = ESP_ERR_INVALID_STATE;
    } else if (mqtt_client == NULL) {
        ESP_LOGE(TAG, "MQTT client not available");
        result = ESP_ERR_INVALID_STATE;
    } else {
        BaseType_t mutex_taken = xSemaphoreTake(s_state.mutex, pdMS_TO_TICKS(5000));

        if (mutex_taken == pdTRUE) {
            uint32_t events_uploaded = 0;

            /* Upload from both log files */
            const char *files[] = {LOG_FILE_0, LOG_FILE_1};

            for (int i = 0; (i < 2) && (result == ESP_OK); i++) {
                FILE *file = fopen(files[i], "r");

                if (file != NULL) {
                    char line[128];

                    /* Read each line and publish to MQTT */
                    while (fgets(line, sizeof(line), file) != NULL) {
                        /* Remove newline */
                        line[strcspn(line, "\n")] = '\0';

                        /* Publish to MQTT */
                        int msg_id = esp_mqtt_client_publish(mqtt_client,
                                                             "node_a/offline_events",
                                                             line,
                                                             0,
                                                             1,
                                                             0);

                        if (msg_id < 0) {
                            ESP_LOGE(TAG, "Failed to publish offline event");
                            result = ESP_FAIL;
                            break;
                        } else {
                            events_uploaded++;
                            /* Small delay to avoid overwhelming broker */
                            vTaskDelay(pdMS_TO_TICKS(50));
                        }
                    }

                    fclose(file);
                } else {
                    ESP_LOGD(TAG, "Log file %s does not exist (OK if no events)", files[i]);
                }
            }

            /* Truncate both files after successful upload */
            if (result == ESP_OK && events_uploaded > 0) {
                FILE *f0 = fopen(LOG_FILE_0, "w");
                FILE *f1 = fopen(LOG_FILE_1, "w");

                if ((f0 != NULL) && (f1 != NULL)) {
                    fclose(f0);
                    fclose(f1);
                    ESP_LOGI(TAG, "Uploaded %lu offline events, logs cleared", events_uploaded);

                    /* Reset upload cursor in NVS */
                    nvs_handle_t nvs_handle = 0;
                    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle) == ESP_OK) {
                        (void)nvs_set_u32(nvs_handle, NVS_KEY_UPLOAD_CURSOR, 0);
                        (void)nvs_commit(nvs_handle);
                        nvs_close(nvs_handle);
                    }
                    s_state.upload_cursor = 0;
                } else {
                    ESP_LOGE(TAG, "Failed to truncate log files after upload");
                    if (f0 != NULL) {
                        fclose(f0);
                    }
                    if (f1 != NULL) {
                        fclose(f1);
                    }
                    result = ESP_FAIL;
                }
            } else if (events_uploaded == 0) {
                ESP_LOGI(TAG, "No offline events to upload");
            } else {
                /* Upload failed */
            }

            xSemaphoreGive(s_state.mutex);
        } else {
            ESP_LOGE(TAG, "Failed to acquire mutex for upload");
            result = ESP_ERR_TIMEOUT;
        }
    }

    return result;
}

uint32_t offline_logger_get_pending_count(void)
{
    uint32_t count = 0;

    if (s_state.initialized) {
        BaseType_t mutex_taken = xSemaphoreTake(s_state.mutex, pdMS_TO_TICKS(1000));

        if (mutex_taken == pdTRUE) {
            /* Count lines in both files */
            const char *files[] = {LOG_FILE_0, LOG_FILE_1};

            for (int i = 0; i < 2; i++) {
                FILE *file = fopen(files[i], "r");

                if (file != NULL) {
                    char line[128];

                    while (fgets(line, sizeof(line), file) != NULL) {
                        if (strlen(line) > 5) {  /* Valid event line */
                            count++;
                        }
                    }

                    fclose(file);
                }
            }

            xSemaphoreGive(s_state.mutex);
        }
    }

    return count;
}

esp_err_t offline_logger_deinit(void)
{
    esp_err_t result = ESP_OK;

    if (s_state.initialized) {
        result = esp_vfs_spiffs_unregister("storage");
        if (result == ESP_OK) {
            ESP_LOGI(TAG, "SPIFFS unmounted");
        } else {
            ESP_LOGE(TAG, "Failed to unmount SPIFFS: %s", esp_err_to_name(result));
        }

        if (s_state.mutex != NULL) {
            vSemaphoreDelete(s_state.mutex);
            s_state.mutex = NULL;
        }

        s_state.initialized = false;
    }

    return result;
}
