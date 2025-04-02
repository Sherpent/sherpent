//
// Created by Mathieu Durand on 2025-03-27.
//

#include "movement.h"
#include <freertos/FreeRTOS.h>
#include <communication.h>
#include <head.h>
#include <math.h>

const uint8_t module_number = 4;
float turn_angle = 0.0f;
float raise_angle = 0.0f;
float slither_frequency = 0.0f;
float sidewinding = 0.0f;

SemaphoreHandle_t slitherMutex;  // Mutex for shared variables

static int8_t angles[MAX_MODULE_NUM][2];

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

int8_t get_angle(enum servo_type_t axis, uint8_t segment_id) {
    return angles[segment_id][axis];
}

void set_axis(enum servo_type_t axis, uint8_t segment_id, int8_t angle) {
    static struct SetYaw yaw_message;   // Allocate once
    static struct SetPitch pitch_message;
    static struct InfoYaw yaw_info;
    static struct InfoPitch pitch_info;

    angles[segment_id][axis] = angle;
    if (!is_segment_id_registered(segment_id))
        return;

    pitch_message.msg_size = (uint8_t) sizeof(struct SetPitch);
    pitch_message.msg_id = SET_PITCH;

    yaw_message.msg_size = (uint8_t) sizeof(struct SetYaw);
    yaw_message.msg_id = SET_YAW;

    pitch_info.msg_size = (uint8_t) sizeof(struct InfoPitch);
    pitch_info.msg_id = INFO_PITCH;

    yaw_info.msg_size = (uint8_t) sizeof(struct InfoYaw);
    yaw_info.msg_id = INFO_YAW;

    switch (axis) {
        case PITCH:
            pitch_message.angle_degrees = angle;
            send_message_to_module(segment_id, (struct Message *) &pitch_message);

            pitch_info.segment_id = segment_id;
            pitch_info.angle_degrees = angle;
            send_message_to_master((struct Message *) &pitch_info);
            break;
        case YAW:
            yaw_message.angle_degrees = angle;
            send_message_to_module(segment_id, (struct Message *) &yaw_message);

            yaw_info.segment_id = segment_id;
            yaw_info.angle_degrees = angle;
            send_message_to_master((struct Message *) &yaw_info);
            break;
    }
}

void roll(float roll_angle, float tightness) {
    float per_module_turn = roll_angle / (float) module_number;
    for (int n = 0; n < module_number; n++) {
        float yaw_angle = per_module_turn * tightness * (float) n / sqrtf(module_number);
        set_axis(YAW, n, yaw_angle);
        set_axis(PITCH, n, 0);
    }
}

void slither() {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const float dt = 100;
    const TickType_t xFrequency = pdMS_TO_TICKS(dt);

    const uint8_t wave_number = 1;
    const float module_offset = (float) wave_number * TWO_PI / (float) module_number;
    const float amplitude = 0.75 * (180.0f / PI) * 9.92899664374f * (float) wave_number / (float) module_number;
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


void init_slither_task() {
    static bool initialized = false;
    if (initialized) return;
    initialized = true;

    slitherMutex = xSemaphoreCreateMutex();
    if (slitherMutex != NULL) {
        xTaskCreate(slither, "SlitherTask", 2048, NULL, 1, NULL);
    }
}
