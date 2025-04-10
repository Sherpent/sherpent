//
// Created by Mathieu Durand on 2025-03-27.
//

#include "movement.h"
#include <freertos/FreeRTOS.h>
#include <communication.h>
#include <head.h>
#include <math.h>
#include <esp_log.h>

const uint8_t module_number = 5;
float turn_angle = 0.0f;
float raise_angle = 0.0f;
float slither_frequency = 0.0f;
float slither_amplitude = 1.0f;
float sidewinding = 0.0f;

SemaphoreHandle_t slitherMutex;  // Mutex for shared variables

static QueueHandle_t angle_queue;

void control_raw(float x1_, float y, float x2_) {
    float x1 = asinf(x1_) * 2.0f / PI;
    float x2 = asinf(x2_) * 2.0f / PI;
    float normalized_x1 = fabsf(x1);

    float frequency = y * (MAX_TURNING_SLITHER_FREQUENCY * normalized_x1 + MAX_FORWARD_SLITHER_FREQUENCY * (1.0f - normalized_x1));
    set_slither_frequency(frequency);

    set_turn_angle(x1 * 180);

    float amplitude = MAX_TURNING_SLITHER_AMPLITUDE * normalized_x1 + MAX_FORWARD_SLITHER_AMPLITUDE * (1.0f - normalized_x1);
    set_slither_amplitude(amplitude);

    set_sidewinding(x2);
}

void set_turn_angle(float angle) {
    init_slither_task();
    if (slitherMutex != NULL && xSemaphoreTake(slitherMutex, portMAX_DELAY)) {
        turn_angle = angle;
        xSemaphoreGive(slitherMutex);
    }
}

void set_raise_angle(float angle) {
    init_slither_task();
    if (slitherMutex != NULL && xSemaphoreTake(slitherMutex, portMAX_DELAY)) {
        raise_angle = angle;
        xSemaphoreGive(slitherMutex);
    }
}

void set_slither_frequency(float frequency) {
    init_slither_task();
    if (slitherMutex != NULL && xSemaphoreTake(slitherMutex, portMAX_DELAY)) {
        slither_frequency = frequency;
        xSemaphoreGive(slitherMutex);
    }
}

void set_slither_amplitude(float amplitude) {
    init_slither_task();
    if (slitherMutex != NULL && xSemaphoreTake(slitherMutex, portMAX_DELAY)) {
        slither_amplitude = fminf(fmaxf(amplitude, -1.0f), 1.0f);
        xSemaphoreGive(slitherMutex);
    }
}

void set_sidewinding(float sidewinding_) {
    init_slither_task();
    if (slitherMutex != NULL && xSemaphoreTake(slitherMutex, portMAX_DELAY)) {
        sidewinding = fminf(fmaxf(sidewinding_, -1.0f), 1.0f);
        xSemaphoreGive(slitherMutex);
    }
}

void set_axis(enum servo_type_t axis, uint8_t segment_id, int8_t angle) {
    if (!is_segment_id_registered(segment_id)) return;
    if (axis >= 2) return;  // prevent out-of-bounds access

    angle_queue_item_t queue_item = {
            .segment_id = segment_id,
            .axis = axis,
            .angle = angle
    };

    QueueHandle_t queue = angle_queue;

    if (queue == NULL) {
        ESP_LOGW("MOVEMENT", "Angle queue not initialized for segment %u, axis %d", segment_id, (int)axis);
        return;
    }

    // Attempt to queue angle
    if (xQueueSendToBack(queue, &queue_item, 0) != pdPASS) {
        ESP_LOGW("MOVEMENT", "Angle queue full for segment %u, axis %d. Overriding oldest value.", segment_id, (int)axis);
        // Drop the oldest entry
        angle_queue_item_t dummy;
        if (xQueueReceive(queue, &dummy, 0) != pdPASS) {
            ESP_LOGE("MOVEMENT", "Failed to dequeue from full queue (segment %u, axis %d)", segment_id, (int)axis);
            return;
        }

        // Try sending again
        if (xQueueSendToBack(queue, &queue_item, 0) != pdPASS) {
            ESP_LOGE("MOVEMENT", "Failed to insert angle after popping (segment %u, axis %d)", segment_id, (int)axis);
        }
    }
}

void roll(float roll_angle, float tightness) {
    float per_module_turn = roll_angle * tightness / (float) module_number;
    for (int n = 0; n < module_number; n++) {
        float yaw_angle = per_module_turn;
        set_axis(YAW, n, yaw_angle);
        set_axis(PITCH, n, 0);
    }
}

void look_up(float look_pitch, float look_yaw) {
    int separation_index = (module_number / 2) + 1;
    int denom = module_number - separation_index;

    if (denom <= 0) return; // Prevent division by zero or negative segment

    float per_segment_raise = -look_pitch / (float)denom;
    float per_segment_yaw = look_yaw / (float)denom;

    for (int n = 0; n < module_number; n++) {
        float pitch_angle = 0;

        if (n < separation_index) {
            pitch_angle = per_segment_raise;
            set_axis(YAW, n, per_segment_yaw);
        }

        if (n == separation_index) {
            pitch_angle += PI / 2.0f;
            set_axis(YAW, n, 45);
        } else if (n > separation_index) {
            int sign = ((n - separation_index) % 2 == 0) ? 1 : -1;
            set_axis(YAW, n, 90.0f * sign);
        }

        set_axis(PITCH, n, pitch_angle);
    }
}

void slither() {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const float dt = 100;
    const TickType_t xFrequency = pdMS_TO_TICKS(dt);

    const uint8_t wave_number = 1;
    const float module_offset = (float) wave_number * TWO_PI / (float) module_number;
    const float amplitude = AMPLITUDE_DAMPENING * (180.0f / PI) * 9.92899664374f * (float) wave_number / (float) module_number;
    const float sidewinding_amplitude = 0.1f * (180.0f / PI) * 9.92899664374f * (float) wave_number / (float) module_number;

    float module_offsets[module_number];
    for (int n = 0; n < module_number; n++) {
        module_offsets[n] = module_offset * (float) n;
    }

    static float slither_theta = 0.0f;

    for (;;) {
        float local_turn_angle = 0.0f, local_raise_angle = 0.0f, local_slither_frequency = 0.0f, local_slither_amplitude = 0.0f, local_sidewinding = 0.0f;

        // Acquire mutex before reading shared variables
        if (xSemaphoreTake(slitherMutex, portMAX_DELAY)) {
            local_turn_angle = turn_angle;
            local_raise_angle = raise_angle;
            local_slither_frequency = slither_frequency;
            local_slither_amplitude = slither_amplitude;
            local_sidewinding = sidewinding;
            xSemaphoreGive(slitherMutex);
        }

        slither_theta = fmodf(slither_theta + (dt / 1000.0f) * TWO_PI * local_slither_frequency, TWO_PI);
        float per_module_turn = local_turn_angle / (float) module_number;
        float per_module_raise = local_raise_angle / (float) module_number;

        for (int n = 0; n < module_number; n++) {
            float yaw_angle = sinf(slither_theta + module_offsets[n]) * local_slither_amplitude * amplitude + per_module_turn;
            set_axis(YAW, n, yaw_angle);

            float pitch_angle = sinf(slither_theta + module_offsets[n] + PI/2.0f) * (local_sidewinding <= 1e-6 ? per_module_raise : (sidewinding_amplitude * local_sidewinding));
            set_axis(PITCH, n, pitch_angle);
        }

        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

void movement_task() {
    angle_queue_item_t queueItem;

    struct SetYaw yaw_message = {
            .msg_size = (uint8_t) sizeof(struct SetYaw),
            .msg_id = SET_YAW
    };
    struct SetPitch pitch_message = {
            .msg_size = (uint8_t) sizeof(struct SetPitch),
            .msg_id = SET_PITCH
    };
    struct InfoYaw yaw_info = {
            .msg_size = (uint8_t) sizeof(struct InfoYaw),
            .msg_id = INFO_YAW
    };
    struct InfoPitch pitch_info = {
            .msg_size = (uint8_t) sizeof(struct InfoPitch),
            .msg_id = INFO_PITCH
    };

    for (;;) {
        bool data_processed = false;
        if (angle_queue == NULL) {
            ESP_LOGW("MOVEMENT", "Angle queue not initialized");
            continue;
        }

        if (xQueueReceive(angle_queue, &queueItem, 10) == pdPASS) {
            if (!is_segment_id_registered(queueItem.segment_id)) continue;
            data_processed = true;

            switch (queueItem.axis) {
                case PITCH:
                    pitch_message.angle_degrees = queueItem.angle;
                    send_message_to_module(queueItem.segment_id, (struct Message *) &pitch_message);

                    pitch_info.segment_id = queueItem.segment_id;
                    pitch_info.angle_degrees = queueItem.angle;
                    send_message_to_master((struct Message *) &pitch_info);
                    break;

                case YAW:
                    yaw_message.angle_degrees = queueItem.angle;
                    send_message_to_module(queueItem.segment_id, (struct Message *) &yaw_message);

                    yaw_info.segment_id = queueItem.segment_id;
                    yaw_info.angle_degrees = queueItem.angle;
                    send_message_to_master((struct Message *) &yaw_info);
                    break;

                default:
                    ESP_LOGW("MOVEMENT", "Invalid axis value: %d", queueItem.segment_id);
                    break;
            }
        }

        if (!data_processed) {
            vTaskDelay(pdMS_TO_TICKS(5));  // Let other tasks run
        }
    }
}

void init_movement() {
    static bool initialized = false;
    if (initialized) return;
    initialized = true;

    angle_queue = xQueueCreate(10 * module_number, sizeof(angle_queue_item_t));

    xTaskCreate(movement_task, "MovementTask", 2048, NULL, 1, NULL);
}

void init_slither_task() {
    static bool initialized = false;
    if (initialized) return;
    initialized = true;

    slitherMutex = xSemaphoreCreateMutex();
    if (slitherMutex != NULL) {
        xTaskCreate(slither, "SlitherTask", 4096, NULL, 1, NULL);
    }
}
