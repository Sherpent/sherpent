//
// Created by Mathieu Durand on 2025-03-27.
//

#include "movement.h"
#include <freertos/FreeRTOS.h>
#include <communication.h>
#include <head.h>
#include <math.h>
#include <esp_log.h>

const uint8_t module_number = 2;
float turn_angle = 0.0f;
float raise_angle = 0.0f;
float slither_frequency = 0.0f;
float sidewinding = 0.0f;

SemaphoreHandle_t slitherMutex;  // Mutex for shared variables

static int8_t angles[MAX_MODULE_NUM][2];
static QueueHandle_t angle_queues[MAX_MODULE_NUM][2];

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

void set_sidewinding(float sidewinding_) {
    init_slither_task();
    if (slitherMutex != NULL && xSemaphoreTake(slitherMutex, portMAX_DELAY)) {
        sidewinding = sidewinding_;
        xSemaphoreGive(slitherMutex);
    }
}

void set_axis(enum servo_type_t axis, uint8_t segment_id, int8_t angle) {
    angle_queue_item_t queue_item = {
            .angle = angle
    };

    QueueHandle_t queue = angle_queues[segment_id][axis];

    if (!is_segment_id_registered(segment_id)) {
        if (queue != NULL) {
            // Clear the queue and insert the latest value as the only message
            xQueueReset(queue);
            xQueueSendToBack(queue, (void *)&queue_item, 0);
        }
        return;
    }

    // Try to send the angle to the queue
    if (queue != NULL) {
        if (xQueueSendToBack(queue, (void *)&queue_item, 0) != pdPASS) {
            ESP_LOGW("MOVEMENT", "Angle queue full, overriding latest value");
            // Remove oldest entry and push the latest value
            xQueueReceive(queue, &queue_item, 0);  // Pop oldest
            xQueueSendToBack(queue, (void *)&queue_item, 0);
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
    float per_segment_raise = -look_pitch / (float) (module_number - separation_index);
    float per_segment_yaw = look_yaw / (float) (module_number - separation_index);
    for (int n = 0; n < module_number; n++) {
        float pitch_angle = 0;
        if (n < separation_index) {
            pitch_angle = per_segment_raise + (n == separation_index ? PI / 2.0f : 0.0f);
            set_axis(YAW, n, per_segment_yaw);
        }
        if (n == separation_index) {
            pitch_angle += (n == separation_index ? PI / 2.0f : 0.0f);
            set_axis(YAW, n, 45);
        } else {
            set_axis(PITCH, n, 90 * powl(-1, n - separation_index));
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
    const float amplitude = 0.5f * (180.0f / PI) * 9.92899664374f * (float) wave_number / (float) module_number;
    const float sidewinding_amplitude = 0.1f * (180.0f / PI) * 9.92899664374f * (float) wave_number / (float) module_number;

    float module_offsets[module_number];
    for (int n = 0; n < module_number; n++) {
        module_offsets[n] = module_offset * (float) n;
    }

    static float slither_theta = 0.0f;

    struct SetYaw yaw_message;   // Allocate once outside the loop
    struct SetPitch pitch_message;

    for (;;) {
        float local_turn_angle = 0.0f, local_raise_angle = 0.0f, local_slither_frequency = 0.0f, local_sidewinding = 0.0f;

        // Acquire mutex before reading shared variables
        if (xSemaphoreTake(slitherMutex, portMAX_DELAY)) {
            local_turn_angle = turn_angle;
            local_raise_angle = raise_angle;
            local_slither_frequency = slither_frequency;
            local_sidewinding = sidewinding;
            xSemaphoreGive(slitherMutex);
        }

        slither_theta = fmodf(slither_theta + (dt / 1000.0f) * TWO_PI * local_slither_frequency, TWO_PI);
        float per_module_turn = local_turn_angle / (float) module_number;
        float per_module_raise = local_raise_angle / (float) module_number;

        for (int n = 0; n < module_number; n++) {
            float yaw_angle = sinf(slither_theta + module_offsets[n]) * amplitude + per_module_turn;
            set_axis(YAW, n, yaw_angle);

            float pitch_angle = sinf(slither_theta + module_offsets[n] + PI/2.0f) * sidewinding_amplitude * local_sidewinding + per_module_raise * (1.0f - fabsf(local_sidewinding));
            set_axis(PITCH, n, pitch_angle);
        }

        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

void movement_task() {
    angle_queue_item_t queueItem;
    struct SetYaw yaw_message;   // Allocate once
    struct SetPitch pitch_message;
    struct InfoYaw yaw_info;
    struct InfoPitch pitch_info;

    pitch_message.msg_size = (uint8_t) sizeof(struct SetPitch);
    pitch_message.msg_id = SET_PITCH;

    yaw_message.msg_size = (uint8_t) sizeof(struct SetYaw);
    yaw_message.msg_id = SET_YAW;

    pitch_info.msg_size = (uint8_t) sizeof(struct InfoPitch);
    pitch_info.msg_id = INFO_PITCH;

    yaw_info.msg_size = (uint8_t) sizeof(struct InfoYaw);
    yaw_info.msg_id = INFO_YAW;

    for (;;) {
        for (int n = 0; n < MAX_MODULE_NUM; n++) {
            if (!is_segment_id_registered(n)) continue;
            for (int axis = 0; axis < 2; axis++) {
                QueueHandle_t queue = angle_queues[n][axis];
                if (queue == NULL) {
                    ESP_LOGW("MOVEMENT", "Angle queue not initialized");
                } else if (xQueueReceive(queue, &queueItem, 10) == pdPASS) {
                    switch (axis) {
                        case PITCH:
                            pitch_message.angle_degrees = queueItem.angle;
                            send_message_to_module(n, (struct Message *) &pitch_message);

                            pitch_info.segment_id = n;
                            pitch_info.angle_degrees = queueItem.angle;
                            send_message_to_master((struct Message *) &pitch_info);
                            break;
                        case YAW:
                            yaw_message.angle_degrees = queueItem.angle;
                            send_message_to_module(n, (struct Message *) &yaw_message);

                            yaw_info.segment_id = n;
                            yaw_info.angle_degrees = queueItem.angle;
                            send_message_to_master((struct Message *) &yaw_info);
                            break;
                    }
                }
            }
        }
    }
}

void init_movement() {
    static bool initialized = false;
    if (initialized) return;
    initialized = true;

    for (int n = 0; n < MAX_MODULE_NUM; n++) {
        for (int i = 0; i < 2; i++) {
            angle_queues[n][i] = xQueueCreate(10, sizeof(angle_queue_item_t));
            if (angle_queues[n][i] == NULL) {
                ESP_LOGE("MOVEMENT", "Failed to create angles queue");
            }
        }
    }

    xTaskCreate(movement_task, "MovementTask", 2048, NULL, 1, NULL);
}

void init_slither_task() {
    static bool initialized = false;
    if (initialized) return;
    initialized = true;

    slitherMutex = xSemaphoreCreateMutex();
    if (slitherMutex != NULL) {
        xTaskCreate(slither, "SlitherTask", 2048, NULL, 1, NULL);
    }
}
