#include "fall_detection_task.h"
#include "config.h"
#include "utils/data_structures.h"
#include "sensors/mpu6050.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <math.h>

static const char *TAG = "FALL_DETECT";

/* ========== Complementary Filter ========== */
static esp_err_t calculate_angles(const mpu6050_data_t *data, angle_t *angles)
{
    static uint8_t first_call = 1U;
    static uint32_t last_time_us = 0U;
    static float prev_pitch = 0.0f;
    static float prev_roll = 0.0f;

    uint32_t now_us = (uint32_t)esp_timer_get_time();
    float dt = (float)(now_us - last_time_us) / 1000000.0f;
    last_time_us = now_us;

    /* Angles from accelerometer */
    float accel_pitch = atan2f(data->accel_x, data->accel_z) * RAD_TO_DEG;
    float accel_roll = atan2f(data->accel_y, data->accel_z) * RAD_TO_DEG;

    if (first_call != 0U) {
        angles->pitch = accel_pitch;
        angles->roll = accel_roll;
        prev_pitch = accel_pitch;
        prev_roll = accel_roll;
        first_call = 0U;
        return ESP_OK;
    } else {
        /* Not first call - apply filter */
    }

    /* Gyroscope contribution (integrate angular velocity) */
    float gyro_pitch_delta = data->gyro_y * dt;
    float gyro_roll_delta = data->gyro_x * dt;

    /* Complementary filter: 99% gyro + 1% accel */
    angles->pitch = (ALPHA * (prev_pitch + gyro_pitch_delta)) + ((1.0f - ALPHA) * accel_pitch);
    angles->roll = (ALPHA * (prev_roll + gyro_roll_delta)) + ((1.0f - ALPHA) * accel_roll);

    prev_pitch = angles->pitch;
    prev_roll = angles->roll;

    return ESP_OK;
}

/* ========== Posture Detection ========== */
static const char *posture_to_string(posture_state_t posture)
{
    const char *result;
    switch (posture) {
        case POSTURE_STANDING: result = "STANDING"; break;
        case POSTURE_SITTING:  result = "SITTING";  break;
        case POSTURE_LYING:    result = "LYING";    break;
        default:               result = "UNKNOWN";  break;
    }
    return result;
}

static posture_state_t detect_posture(const angle_t *angles, posture_detector_t *detector)
{
    posture_state_t new_state;
    float abs_pitch = fabsf(angles->pitch);
    float abs_roll = fabsf(angles->roll);

    /* Priority: LYING > SITTING > STANDING */
    if ((abs_pitch <= LYING_PITCH_MAX) && (abs_roll <= LYING_PITCH_MAX)) {
        new_state = POSTURE_LYING;
    } else if (abs_pitch <= STANDING_PITCH_MAX) {
        new_state = POSTURE_STANDING;
    } else if (abs_pitch >= SITTING_PITCH_MIN) {
        new_state = POSTURE_SITTING;
    } else {
        new_state = detector->current_state;  /* Dead zone */
    }

    /* Hysteresis: require stable samples before transition */
    if (new_state == detector->previous_state) {
        detector->state_stable_count++;

        if (detector->state_stable_count >= HYSTERESIS_COUNT) {
            if (new_state != detector->current_state) {
                ESP_LOGI(TAG, "POSTURE: %s -> %s (Pitch: %.1f, Roll: %.1f)",
                         posture_to_string(detector->current_state),
                         posture_to_string(new_state),
                         angles->pitch, angles->roll);
                detector->current_state = new_state;
            } else {
                /* Same state */
            }
        } else {
            /* Still counting */
        }
    } else {
        detector->state_stable_count = 0U;
        detector->previous_state = new_state;
    }

    return detector->current_state;
}

/* ========== Jerk-Based Fall Detection ========== */
static fall_detection_result_t detect_fall(const mpu6050_data_t *data)
{
    static float prev_accel_x = 0.0f;
    static float prev_accel_y = 0.0f;
    static float prev_accel_z = 0.0f;
    static uint8_t first_read = 1U;
    static uint32_t last_fall_time_ms = 0U;

    fall_detection_result_t result = {0U, 0.0f};

    /* Convert to g-force */
    float accel_g_x = data->accel_x / GRAVITY;
    float accel_g_y = data->accel_y / GRAVITY;
    float accel_g_z = data->accel_z / GRAVITY;

    if (first_read != 0U) {
        prev_accel_x = accel_g_x;
        prev_accel_y = accel_g_y;
        prev_accel_z = accel_g_z;
        first_read = 0U;
        return result;
    } else {
        /* Not first read */
    }

    /* Calculate jerk (rate of change of acceleration) in g/s */
    float jerk_x = (accel_g_x - prev_accel_x) / SAMPLE_INTERVAL_SEC;
    float jerk_y = (accel_g_y - prev_accel_y) / SAMPLE_INTERVAL_SEC;
    float jerk_z = (accel_g_z - prev_accel_z) / SAMPLE_INTERVAL_SEC;

    prev_accel_x = accel_g_x;
    prev_accel_y = accel_g_y;
    prev_accel_z = accel_g_z;

    float jerk_magnitude = sqrtf((jerk_x * jerk_x) + (jerk_y * jerk_y) + (jerk_z * jerk_z));
    result.jerk_magnitude = jerk_magnitude;

    /* Cooldown check */
    uint32_t current_time_ms = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
    uint32_t time_since_last_fall = current_time_ms - last_fall_time_ms;

    if ((time_since_last_fall < (uint32_t)FALL_COOLDOWN_MS) && (last_fall_time_ms > 0U)) {
        if (jerk_magnitude > FALL_JERK_THRESHOLD) {
            ESP_LOGD(TAG, "Jerk %.0f g/s in cooldown (%lu ms left)",
                     jerk_magnitude, (unsigned long)((uint32_t)FALL_COOLDOWN_MS - time_since_last_fall));
        } else {
            /* Below threshold */
        }
        return result;
    } else {
        /* Not in cooldown */
    }

    if (jerk_magnitude > FALL_JERK_THRESHOLD) {
        last_fall_time_ms = current_time_ms;

        ESP_LOGW(TAG, "========================================");
        ESP_LOGW(TAG, "          FALL DETECTED!");
        ESP_LOGW(TAG, "========================================");
        ESP_LOGW(TAG, "Jerk: %.0f g/s (threshold: %.0f g/s)",
                 jerk_magnitude, FALL_JERK_THRESHOLD);
        ESP_LOGW(TAG, "Accel: (%.2f, %.2f, %.2f) g",
                 accel_g_x, accel_g_y, accel_g_z);
        ESP_LOGW(TAG, "========================================");
        result.fall_detected = 1U;
    } else {
        /* No fall */
    }

    return result;
}

/* ========== Fall Detection Task ========== */
void fall_detection_task(void *pvParameters)
{
    (void)pvParameters;  /* MISRA Rule 2.7 */

    ESP_LOGI(TAG, "Fall detection task started (jerk-based, 100Hz)");

    mpu6050_data_t mpu_data;
    angle_t angles = {0};
    posture_detector_t posture_detector = {
        .current_state = POSTURE_UNKNOWN,
        .previous_state = POSTURE_UNKNOWN,
        .state_stable_count = 0U
    };

    /* Warm-up: let sensor stabilize */
    ESP_LOGI(TAG, "Warming up sensor (2 seconds)...");
    for (int i = 0; i < 200; i++) {
        if (mpu6050_read(&mpu_data) == ESP_OK) {
            (void)calculate_angles(&mpu_data, &angles);
        } else {
            /* Read failed, skip */
        }
        vTaskDelay(pdMS_TO_TICKS(SAMPLE_INTERVAL_MS));
    }

    ESP_LOGI(TAG, "Sensor ready - monitoring for falls and posture");

    uint32_t sample_count = 0U;

    while (1) {
        esp_err_t ret = mpu6050_read(&mpu_data);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to read MPU6050");
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        } else {
            /* Read successful */
        }

        /* Calculate angles */
        (void)calculate_angles(&mpu_data, &angles);

        /* Detect posture */
        (void)detect_posture(&angles, &posture_detector);

        /* Detect falls */
        fall_detection_result_t fall_result = detect_fall(&mpu_data);

        if (fall_result.fall_detected != 0U) {
            ESP_LOGW(TAG, "FALL while %s (Pitch: %.1f)",
                     posture_to_string(posture_detector.current_state),
                     angles.pitch);

            (void)xEventGroupSetBits(emergency_event_group, FALL_DETECTED_BIT);
        } else {
            /* No fall */
        }

        /* Debug log every 100 samples (~1s) */
        sample_count++;
        if ((sample_count % 100U) == 0U) {
            ESP_LOGI(TAG, "Posture: %s | Pitch: %.1f° | Roll: %.1f° | Jerk: %.0f g/s",
                     posture_to_string(posture_detector.current_state),
                     angles.pitch,
                     angles.roll,
                     fall_result.jerk_magnitude);
        } else {
            /* Not time to log */
        }

        vTaskDelay(pdMS_TO_TICKS(SAMPLE_INTERVAL_MS));
    }

    vTaskDelete(NULL);
}
