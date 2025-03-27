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

SemaphoreHandle_t slitherMutex;  // Mutex for shared variables

void set_turn_angle(float angle) {
    if (slitherMutex != NULL && xSemaphoreTake(slitherMutex, portMAX_DELAY)) {
        turn_angle = angle;
        xSemaphoreGive(slitherMutex);
    }
}

void set_raise_angle(float angle) {
    if (slitherMutex != NULL && xSemaphoreTake(slitherMutex, portMAX_DELAY)) {
        raise_angle = angle;
        xSemaphoreGive(slitherMutex);
    }
}

void set_slither_frequency(float frequency) {
    if (slitherMutex != NULL && xSemaphoreTake(slitherMutex, portMAX_DELAY)) {
        slither_frequency = frequency;
        xSemaphoreGive(slitherMutex);
    }
}

void slither() {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const float dt = 100;
    const TickType_t xFrequency = pdMS_TO_TICKS(dt);

    const uint8_t wave_number = 1;
    const float module_offset = (float) wave_number * TWO_PI / (float) module_number;
    const float amplitude = 0.75 * (180.0f / PI) * 9.92899664374f * (float) wave_number / (float) module_number;

    float module_offsets[module_number];
    for (int n = 0; n < module_number; n++) {
        module_offsets[n] = module_offset * (float) n;
    }

    static float slither_theta = 0.0f;

    struct SetYaw yaw_message;   // Allocate once outside the loop
    struct SetPitch pitch_message;

    for (;;) {
        float local_turn_angle = 0.0f, local_raise_angle = 0.0f, local_slither_frequency = 0.0f;

        // Acquire mutex before reading shared variables
        if (xSemaphoreTake(slitherMutex, portMAX_DELAY)) {
            local_turn_angle = turn_angle;
            local_raise_angle = raise_angle;
            local_slither_frequency = slither_frequency;
            xSemaphoreGive(slitherMutex);
        }

        slither_theta = fmodf(slither_theta + (dt / 1000.0f) * TWO_PI * local_slither_frequency, TWO_PI);
        float per_module_turn = local_turn_angle / (float) module_number;
        float per_module_raise = local_raise_angle / (float) module_number;

        for (int n = 0; n < module_number; n++) {
            float yaw_angle = sinf(slither_theta + module_offsets[n]) * amplitude + per_module_turn;

            yaw_message.msg_size = (uint8_t) sizeof(struct SetYaw);
            yaw_message.msg_id = SET_YAW;
            yaw_message.angle_degrees = yaw_angle;

            send_message_to_module(n, (struct Message *) &yaw_message);

            pitch_message.msg_size = (uint8_t) sizeof(struct SetPitch);
            pitch_message.msg_id = SET_PITCH;
            pitch_message.angle_degrees = per_module_raise;

            send_message_to_module(n, (struct Message *) &pitch_message);
        }

        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

void init_slither_task() {
    slitherMutex = xSemaphoreCreateMutex();
    if (slitherMutex != NULL) {
        xTaskCreate(slither, "SlitherTask", 2048, NULL, 1, NULL);
    }
}
