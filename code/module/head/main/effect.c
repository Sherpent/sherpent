//
// Created by Mathieu Durand on 2025-04-10.
//

#include "effect.h"
#include <led.h>
#include <math.h>
#include "communication.h"
#include <head.h>
#include <movement.h>

static TaskHandle_t rainbow_handle = NULL;

void rainbow_task() {
    TickType_t last_tick = xTaskGetTickCount();
    const float offset = M_TWOPI / 3;
    float angle = 0;

    TickType_t xFrequency = 1000 / portTICK_PERIOD_MS;

    struct SetLightGradient light_message = {
            .msg_size = (uint8_t) sizeof(struct SetLightGradient),
            .msg_id = SET_LIGHT_GRADIENT,
            .type = SPIRAL
    };

    const float per_module_offset = M_PI / MAX_MODULE_NUM;

    for (;;) {
        TickType_t now = xTaskGetTickCount();
        float dt = (float)(now - last_tick) * portTICK_PERIOD_MS;  // ms
        last_tick = now;

        angle += M_TWOPI * ((float) dt / 1000.0f) / 3.0f;
        for (int n = 0; n < MAX_MODULE_NUM; n++) {
            float start_offset = per_module_offset * ((float) n - 0.5f);
            light_message.start_red = sinf(angle - start_offset) * 127 + 128;
            light_message.start_green = sinf(angle + offset - start_offset) * 127 + 128;
            light_message.start_blue = sinf(angle + 2.0f * offset - start_offset) * 127 + 128;

            float end_offset = per_module_offset * ((float) n + 0.5f);
            light_message.end_red = sinf(angle - end_offset) * 127 + 128;
            light_message.end_green = sinf(angle + offset - end_offset) * 127 + 128;
            light_message.end_blue = sinf(angle + 2.0f * offset - end_offset) * 127 + 128;

            send_message_to_module(n, (struct Message *) &light_message);
        }

        vTaskDelay(xFrequency);
    }
}

void start_rainbow() {
    if (rainbow_handle == NULL) {
        xTaskCreate(rainbow_task, "Rainbow", 2048, NULL, 1, &rainbow_handle);
    } else {
        vTaskResume(rainbow_handle);
    }
}

void stop_rainbow() {
    if (rainbow_handle != NULL) {
        vTaskSuspend(rainbow_handle);
    }
}
