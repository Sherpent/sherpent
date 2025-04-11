//
// Created by Mathieu Durand on 2025-03-27.
//

#include "movement.h"
#include <freertos/FreeRTOS.h>
#include <communication.h>
#include <head.h>
#include <math.h>
#include <esp_log.h>

float turn_angle = 0.0f;
float raise_angle = 0.0f;
float slither_frequency = 0.0f;
float slither_amplitude = 1.0f;
float sidewinding = 0.0f;

SemaphoreHandle_t slitherMutex;  // Mutex for shared variables

static QueueHandle_t angle_queue;

static int8_t angles[MAX_MODULE_NUM][2];

void control_raw(float x1_, float y, float x2_, float mouth) {
    float x1 = asinf(x1_) * 2.0f / PI;
    float x2 = asinf(x2_) * 2.0f / PI;
    float normalized_x1 = fabsf(x1);

    float frequency = y * (MAX_TURNING_SLITHER_FREQUENCY * normalized_x1 + MAX_FORWARD_SLITHER_FREQUENCY * (1.0f - normalized_x1));
    set_slither_frequency(frequency);

    set_turn_angle(x1 * 360);

    float amplitude = MAX_TURNING_SLITHER_AMPLITUDE * normalized_x1 + MAX_FORWARD_SLITHER_AMPLITUDE * (1.0f - normalized_x1);
    set_slither_amplitude(amplitude);

    set_sidewinding(x2);

    float mouth_angle = MOUTH_OPENED_ANGLE * mouth + MOUTH_CLOSED_ANGLE * (1.0f  - mouth);
    set_axis_direct(PITCH, 0, mouth_angle);
}

int8_t get_angle(enum servo_type_t axis, uint8_t segment_id) {
    return angles[segment_id][axis];
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

void set_both_axis(uint8_t segment_id, int8_t pitch_angle, int8_t yaw_angle) {
    if (!is_segment_id_registered(segment_id)) return;

    angle_queue_item_t queue_item = {
            .segment_id = segment_id,
            .pitch = true,
            .yaw = true,
            .pitch_angle = pitch_angle,
            .yaw_angle = yaw_angle,
    };

    QueueHandle_t queue = angle_queue;

    if (queue == NULL) {
        ESP_LOGW("MOVEMENT", "Angle queue not initialized for segment %u", segment_id);
        return;
    }

    // Attempt to queue angle
    if (xQueueSendToBack(queue, &queue_item, 0) != pdPASS) {
        ESP_LOGW("MOVEMENT", "Angle queue full for segment %u. Overriding oldest value.", segment_id);
        // Drop the oldest entry
        angle_queue_item_t dummy;
        if (xQueueReceive(queue, &dummy, 0) != pdPASS) {
            ESP_LOGE("MOVEMENT", "Failed to dequeue from full queue (segment %u)", segment_id);
            return;
        }

        // Try sending again
        if (xQueueSendToBack(queue, &queue_item, 0) != pdPASS) {
            ESP_LOGE("MOVEMENT", "Failed to insert angle after popping (segment %u)", segment_id);
        }
    }
}
void set_axis(enum servo_type_t axis, uint8_t segment_id, int8_t angle) {
    if (!is_segment_id_registered(segment_id)) return;
    if (axis >= 2) return;  // prevent out-of-bounds access

    angle_queue_item_t queue_item = {
            .segment_id = segment_id,
            .pitch = axis == PITCH,
            .yaw = axis == YAW,
            .pitch_angle = angle,
            .yaw_angle = angle
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

void set_both_axis_direct(uint8_t segment_id, int8_t pitch_angle, int8_t yaw_angle) {
    struct SetPitchYaw pitch_yaw_message = {
            .msg_size = (uint8_t) sizeof(struct SetPitchYaw),
            .msg_id = SET_PITCH_YAW
    };
    /*
    struct InfoYaw yaw_info = {
            .msg_size = (uint8_t) sizeof(struct InfoYaw),
            .msg_id = INFO_YAW
    };
    struct InfoPitch pitch_info = {
            .msg_size = (uint8_t) sizeof(struct InfoPitch),
            .msg_id = INFO_PITCH
    };
    struct InfoPitchYaw pitch_yaw_info = {
            .msg_size = (uint8_t) sizeof(struct InfoPitchYaw),
            .msg_id = INFO_PITCH_YAW
    };
    */

    pitch_yaw_message.pitch_degrees = pitch_angle;
    pitch_yaw_message.yaw_degrees = yaw_angle;
    send_message_to_module(segment_id, (struct Message *) &pitch_yaw_message);

    //pitch_yaw_info.segment_id = segment_id;
    //pitch_yaw_info.pitch_degrees = pitch_angle;
    //pitch_yaw_info.yaw_degrees = yaw_angle;
    //send_message_to_master((struct Message *) &pitch_yaw_info);
    angles[segment_id][PITCH] = pitch_angle;
    angles[segment_id][YAW] = yaw_angle;
}
void set_axis_direct(enum servo_type_t axis, uint8_t segment_id, int8_t angle) {
    struct SetYaw yaw_message = {
            .msg_size = (uint8_t) sizeof(struct SetYaw),
            .msg_id = SET_YAW
    };
    struct SetPitch pitch_message = {
            .msg_size = (uint8_t) sizeof(struct SetPitch),
            .msg_id = SET_PITCH
    };
    /*
    struct InfoYaw yaw_info = {
            .msg_size = (uint8_t) sizeof(struct InfoYaw),
            .msg_id = INFO_YAW
    };
    struct InfoPitch pitch_info = {
            .msg_size = (uint8_t) sizeof(struct InfoPitch),
            .msg_id = INFO_PITCH
    };
    */

    switch (axis) {
        case PITCH: {
            pitch_message.angle_degrees = angle;
            send_message_to_module(segment_id, (struct Message *) &pitch_message);

            //pitch_info.segment_id = segment_id;
            //pitch_info.angle_degrees = angle;
            //send_message_to_master((struct Message *) &pitch_info);
            angles[segment_id][PITCH] = angle;
            break;
        }
        case YAW: {
            yaw_message.angle_degrees = angle;
            send_message_to_module(segment_id, (struct Message *) &yaw_message);

            //yaw_info.segment_id = segment_id;
            //yaw_info.angle_degrees = angle;
            //send_message_to_master((struct Message *) &yaw_info);
            angles[segment_id][YAW] = angle;
            break;
        }
    }
}

void roll(float roll_angle, float tightness) {
    float per_module_turn = roll_angle * tightness / (float) MAX_MODULE_NUM;
    for (int n = 0; n < MAX_MODULE_NUM; n++) {
        float yaw_angle = per_module_turn;
        set_axis(YAW, n, yaw_angle);
        set_axis(PITCH, n, 0);
    }
}

void look_up(float look_pitch, float look_yaw) {
    int separation_index = (MAX_MODULE_NUM / 2);
    int denom = MAX_MODULE_NUM - separation_index;

    if (denom <= 0) return; // Prevent division by zero or negative segment

    float per_segment_raise = -look_pitch / (float)denom;
    float per_segment_yaw = look_yaw / (float)denom;

    for (int n = 0; n < MAX_MODULE_NUM; n++) {
        float pitch_angle = 0;

        if (n < separation_index) {
            pitch_angle = -per_segment_raise;
            set_axis_direct(YAW, n, per_segment_yaw);
        }

        if (n == separation_index) {
            pitch_angle += 90.0f;
            set_axis_direct(YAW, n, 45);
        } else if (n - 1 > separation_index) {
            int sign = ((n - separation_index) % 2 == 0) ? 1 : -1;
            set_axis_direct(YAW, n, 90.0f * sign);
        }

        if (n != 0) {
            set_axis_direct(PITCH, n, pitch_angle);
        }
    }
}

void slither() {
    TickType_t last_tick = xTaskGetTickCount();
    TickType_t xFrequency = 50 / portTICK_PERIOD_MS;
    float slither_theta = 0.0f;

    const uint8_t wave_number = 1;
    const float module_offset = (float) wave_number * TWO_PI / (float) MAX_MODULE_NUM;
    const float amplitude = AMPLITUDE_DAMPENING * (180.0f / PI) * 9.92899664374f * (float) wave_number / (float) MAX_MODULE_NUM;
    const float sidewinding_amplitude = 0.1f * (180.0f / PI) * 9.92899664374f * (float) wave_number / (float) MAX_MODULE_NUM;

    float module_offsets[MAX_MODULE_NUM];
    for (int n = 0; n < MAX_MODULE_NUM; n++) {
        module_offsets[n] = module_offset * (float) n;
    }

    for (;;) {
        TickType_t now = xTaskGetTickCount();
        float dt = (float)(now - last_tick) * portTICK_PERIOD_MS;  // ms
        last_tick = now;

        float local_turn_angle = 0.0f, local_raise_angle = 0.0f;
        float local_slither_frequency = 0.0f, local_slither_amplitude = 0.0f, local_sidewinding = 0.0f;

        // Acquire mutex before reading shared variables
        if (xSemaphoreTake(slitherMutex, portMAX_DELAY)) {
            local_turn_angle = turn_angle;
            local_raise_angle = raise_angle;
            local_slither_frequency = slither_frequency;
            local_slither_amplitude = slither_amplitude;
            local_sidewinding = sidewinding;
            xSemaphoreGive(slitherMutex);
        }

        // Advance theta using actual elapsed time
        slither_theta = fmodf(slither_theta + (dt / 1000.0f) * TWO_PI * -local_slither_frequency, TWO_PI);
        float per_module_turn = local_turn_angle / (float) MAX_MODULE_NUM;
        float per_module_raise = local_raise_angle / (float) MAX_MODULE_NUM;

        for (int n = 0; n < MAX_MODULE_NUM; n++) {
            float yaw_angle = sinf(slither_theta + module_offsets[n]) * local_slither_amplitude * amplitude + per_module_turn;
            float pitch_angle = sinf(slither_theta + module_offsets[n] + PI / 2.0f) *
                                (fabsf(local_sidewinding) <= 1e-6f ? per_module_raise : (sidewinding_amplitude * -local_sidewinding));

            if (n == 0) {
                set_axis_direct(YAW, n, yaw_angle);
            } else {
                set_both_axis_direct(n, pitch_angle, yaw_angle);
            }
        }

        // Yield to allow lower-priority tasks some CPU time
        vTaskDelay(xFrequency);
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
    struct SetPitchYaw pitch_yaw_message = {
            .msg_size = (uint8_t) sizeof(struct SetPitchYaw),
            .msg_id = SET_PITCH_YAW
    };
    struct InfoYaw yaw_info = {
            .msg_size = (uint8_t) sizeof(struct InfoYaw),
            .msg_id = INFO_YAW
    };
    struct InfoPitch pitch_info = {
            .msg_size = (uint8_t) sizeof(struct InfoPitch),
            .msg_id = INFO_PITCH
    };
    struct InfoPitchYaw pitch_yaw_info = {
            .msg_size = (uint8_t) sizeof(struct InfoPitchYaw),
            .msg_id = INFO_PITCH_YAW
    };

    for (;;) {
        if (angle_queue == NULL) {
            ESP_LOGW("MOVEMENT", "Angle queue not initialized");
            continue;
        }

        for (int i = 0; i < 10; i++) {
            if (xQueueReceive(angle_queue, &queueItem, 10) == pdPASS) {
                if (!is_segment_id_registered(queueItem.segment_id)) continue;

                if (queueItem.pitch && queueItem.yaw) {
                    pitch_yaw_message.pitch_degrees = queueItem.pitch_angle;
                    pitch_yaw_message.yaw_degrees = queueItem.yaw_angle;
                    send_message_to_module(queueItem.segment_id, (struct Message *) &pitch_yaw_message);

                    pitch_yaw_info.segment_id = queueItem.segment_id;
                    pitch_yaw_info.pitch_degrees = queueItem.pitch_angle;
                    pitch_yaw_info.yaw_degrees = queueItem.yaw_angle;
                    send_message_to_master((struct Message *) &pitch_yaw_info);
                } else if (queueItem.pitch) {
                    pitch_message.angle_degrees = queueItem.pitch_angle;
                    send_message_to_module(queueItem.segment_id, (struct Message *) &pitch_message);

                    pitch_info.segment_id = queueItem.segment_id;
                    pitch_info.angle_degrees = queueItem.pitch_angle;
                    send_message_to_master((struct Message *) &pitch_info);
                } else if (queueItem.yaw) {
                    yaw_message.angle_degrees = queueItem.yaw_angle;
                    send_message_to_module(queueItem.segment_id, (struct Message *) &yaw_message);

                    yaw_info.segment_id = queueItem.segment_id;
                    yaw_info.angle_degrees = queueItem.yaw_angle;
                    send_message_to_master((struct Message *) &yaw_info);
                }
            } else {
                break;
            }
        }

        taskYIELD();
    }
}

void init_movement() {
    static bool initialized = false;
    if (initialized) return;
    initialized = true;

    angle_queue = xQueueCreate(10 * MAX_MODULE_NUM, sizeof(angle_queue_item_t));

    //xTaskCreate(movement_task, "MovementTask", 2048, NULL, 1, NULL);
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
