#include "security_task.h"
#include "config.h"
#include "utils/data_structures.h"
#include "modules/rfid_rc522.h"
#include "modules/ir_sensor.h"
#include "modules/buzzer.h"
#include "esp_log.h"

static const char *TAG = "SECURITY";

/* Forward declaration for MQTT publish (defined in mqtt_task.c) */
extern void mqtt_publish_security(const char *topic, const char *data);

void security_task(void *pvParameters)
{
    (void)pvParameters;  /* MISRA Rule 2.7 */

    ESP_LOGI(TAG, "Security task started");
    ESP_LOGI(TAG, "Security Logic:");
    ESP_LOGI(TAG, "  - IR detected -> 5-second window starts");
    ESP_LOGI(TAG, "  - RFID within 5s -> Entry allowed");
    ESP_LOGI(TAG, "  - No RFID after 5s -> INTRUSION (buzzer)");

    security_state_t current_state = STATE_IDLE;
    TickType_t ir_detection_start = 0U;
    TickType_t state_log_timer = 0U;

    while (1) {
        bool ir_active = ir_sensor_is_detected();

        switch (current_state) {
            case STATE_IDLE:
                if (ir_active) {
                    ESP_LOGI(TAG, "IR DETECTED - Starting 5-second verification window");
                    mqtt_publish_security("security/status", "IR_DETECTED");
                    current_state = STATE_IR_DETECTED;
                    ir_detection_start = xTaskGetTickCount();
                    state_log_timer = xTaskGetTickCount();
                } else {
                    /* No IR detection */
                }
                break;

            case STATE_IR_DETECTED:
                if (!ir_active) {
                    /* IR removed before timeout - back to idle */
                    ESP_LOGI(TAG, "IR removed - returning to idle state");
                    mqtt_publish_security("security/status", "IDLE");
                    current_state = STATE_IDLE;
                    break;
                } else {
                    /* IR still active */
                }

                /* Check for RFID card */
                {
                    rfid_result_t rfid_check = rfid_rc522_check_card();
                    if (rfid_check.detected) {
                        if (rfid_check.authorized) {
                            /* AUTHORIZED - Allow entry */
                            ESP_LOGI(TAG, "========================================");
                            ESP_LOGI(TAG, "ENTRY ALLOWED");
                            ESP_LOGI(TAG, "  Card: %s", rfid_check.name);
                            ESP_LOGI(TAG, "========================================");
                            mqtt_publish_security("security/status", "AUTHORIZED");

                            /* Publish UID to MQTT */
                            char rfid_msg[64];
                            (void)snprintf(rfid_msg, sizeof(rfid_msg),
                                     "{\"card\":\"%s\",\"authorized\":true}", rfid_check.name);
                            mqtt_publish_security("rfid/scanned", rfid_msg);

                            current_state = STATE_AUTHORIZED;
                            state_log_timer = xTaskGetTickCount();
                        } else {
                            /* UNAUTHORIZED - Trigger intrusion alarm */
                            ESP_LOGW(TAG, "========================================");
                            ESP_LOGW(TAG, "UNAUTHORIZED CARD DETECTED!");
                            ESP_LOGW(TAG, "  Card: %s", rfid_check.name);
                            ESP_LOGW(TAG, "  INTRUSION ALARM ACTIVATED");
                            ESP_LOGW(TAG, "========================================");

                            char rfid_msg[64];
                            (void)snprintf(rfid_msg, sizeof(rfid_msg),
                                     "{\"card\":\"%s\",\"authorized\":false}", rfid_check.name);
                            mqtt_publish_security("rfid/scanned", rfid_msg);

                            current_state = STATE_INTRUSION;
                            buzzer_on();
                            mqtt_publish_security("security/alarm", "INTRUSION");
                            state_log_timer = xTaskGetTickCount();
                        }
                        break;
                    } else {
                        /* No card detected yet */
                    }
                }

                /* Check if 5-second window has expired */
                {
                    TickType_t elapsed = (TickType_t)((xTaskGetTickCount() - ir_detection_start) * portTICK_PERIOD_MS);
                    if (elapsed >= (TickType_t)IR_WINDOW_MS) {
                        /* Timeout without RFID - intrusion! */
                        ESP_LOGW(TAG, "========================================");
                        ESP_LOGW(TAG, "INTRUSION DETECTED");
                        ESP_LOGW(TAG, "  No RFID within 5 seconds!");
                        ESP_LOGW(TAG, "  INTRUSION ALARM ACTIVATED");
                        ESP_LOGW(TAG, "========================================");
                        current_state = STATE_INTRUSION;
                        buzzer_on();
                        mqtt_publish_security("security/alarm", "INTRUSION");
                        state_log_timer = xTaskGetTickCount();
                    } else {
                        /* Still within window */
                    }
                }
                break;

            case STATE_AUTHORIZED:
                if (!ir_active) {
                    /* Person has passed through - return to idle */
                    ESP_LOGI(TAG, "Authorized person cleared - returning to monitoring");
                    mqtt_publish_security("security/status", "IDLE");
                    current_state = STATE_IDLE;
                } else {
                    /* Log status every 2 seconds while authorized */
                    TickType_t log_elapsed = (TickType_t)((xTaskGetTickCount() - state_log_timer) * portTICK_PERIOD_MS);
                    if (log_elapsed >= 2000U) {
                        ESP_LOGI(TAG, "Status: Authorized entry in progress - IR still active");
                        state_log_timer = xTaskGetTickCount();
                    } else {
                        /* Not time to log */
                    }
                }
                break;

            case STATE_INTRUSION:
                if (!ir_active) {
                    /* Intruder has left - turn off buzzer and return to idle */
                    ESP_LOGI(TAG, "IR cleared - intrusion ended, returning to monitoring");
                    buzzer_off();
                    mqtt_publish_security("security/alarm", "OFF");
                    mqtt_publish_security("security/status", "IDLE");
                    current_state = STATE_IDLE;
                } else {
                    /* Log status every 2 seconds during intrusion */
                    TickType_t log_elapsed = (TickType_t)((xTaskGetTickCount() - state_log_timer) * portTICK_PERIOD_MS);
                    if (log_elapsed >= 2000U) {
                        ESP_LOGW(TAG, "Status: INTRUSION ONGOING - Buzzer active, IR still detected");
                        state_log_timer = xTaskGetTickCount();
                    } else {
                        /* Not time to log */
                    }
                }
                break;

            default:
                current_state = STATE_IDLE;
                break;
        }

        /* Adjust delay based on state */
        if (current_state == STATE_IR_DETECTED) {
            vTaskDelay(pdMS_TO_TICKS(RFID_POLL_MS));   /* Poll RFID quickly */
        } else {
            vTaskDelay(pdMS_TO_TICKS(IR_CHECK_MS));     /* Normal IR monitoring */
        }
    }

    vTaskDelete(NULL);
}
