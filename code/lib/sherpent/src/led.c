//
// Created by Mathieu Durand on 2025-02-24.
//

#include <soc/io_mux_reg.h>
#include <neopixel.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "led.h"
#include "esp_log.h"

static tNeopixelContext neopixel = NULL;

static uint32_t main_color[PIXEL_COUNT] = {0};
static QueueHandle_t effect_queue[PIXEL_COUNT];
static TaskHandle_t led_task = NULL;

void led_init() {
    bool queue_creation_failed = false;
    for (int i = 0; i < PIXEL_COUNT; i++) {
        effect_queue[i] = xQueueCreate(COLOR_QUEUE_SIZE, sizeof(uint32_t));
        if (effect_queue[i] == NULL) {
            queue_creation_failed = true;
            break;
        }
    }

    if (queue_creation_failed) {
        ESP_LOGE("LED", "Failed to create effect queues");
    } else {
        xTaskCreate(task_led, "LEDTask", 2048, NULL, 1, &led_task);
    }

    neopixel = neopixel_Init(PIXEL_COUNT, NEOPIXEL_PIN);
    xTaskNotifyGive(led_task); // Update LEDs
}

void set_pixel_rgb(uint32_t pixel_num, uint8_t red, uint8_t green, uint8_t blue) {
    if (pixel_num >= PIXEL_COUNT) {
        ESP_LOGW("LED", "Trying to set color for invalid pixel index, %d, Valid pixel index are in the range [0, %d]", pixel_num, PIXEL_COUNT - 1);
        return;
    }

    main_color[pixel_num] = NP_RGB(red, green, blue);
    xTaskNotifyGive(led_task);
}

void set_pixel_effect_rgb(uint32_t pixel_num, uint8_t red, uint8_t green, uint8_t blue) {
    if (pixel_num >= PIXEL_COUNT) {
        ESP_LOGW("LED", "Trying to set effect color for invalid pixel index, %d, Valid pixel index are in the range [0, %d]", pixel_num, PIXEL_COUNT - 1);
        return;
    }

    uint32_t color = NP_RGB(red, green, blue);
    if (xQueueSendToFront(effect_queue[pixel_num], (void *) &color, 10) != pdTRUE) {
        ESP_LOGW("LED", "Failed to push led state for queue #%d", pixel_num);
    } else {
        xTaskNotifyGive(led_task);
    }
}

void set_pixel_effect_release(uint32_t pixel_num) {
    if (pixel_num >= PIXEL_COUNT) {
        ESP_LOGW("LED", "Trying to release effect color for invalid pixel index, %d, Valid pixel index are in the range [0, %d]", pixel_num, PIXEL_COUNT - 1);
        return;
    }

    uint32_t color;
    xQueueReceive(effect_queue[pixel_num], &color, 10);
    xTaskNotifyGive(led_task);
}

void task_led(void *parameters) {
    uint32_t color;

    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        ESP_LOGI("LED", "Updating LEDs");
        for (int i = 0; i < PIXEL_COUNT; i++) {
            if (uxQueueMessagesWaiting(effect_queue[i]) == 0) {
                color = main_color[i];
            } else {
                xQueuePeek(effect_queue[i], &color, 10);
            }

            tNeopixel pixel = {i, color};
            if (!neopixel_SetPixel(neopixel, &pixel, 1)) ESP_LOGE("LED", "Failed to control LEDs");
        }
    }
}

void task_flash(void *parameters) {
    struct FlashParams *params = (struct FlashParams *) parameters;

    // Ensure duty is between 0 and 1
    if (params->duty < 0.0f) {
        params->duty = 0.0f;
    } else if (params->duty > 1.0f) {
        params->duty = 1.0f;
    }

    // Convert period and duty to ticks
    TickType_t high_period = pdMS_TO_TICKS((float) params->period * params->duty);
    TickType_t low_period = pdMS_TO_TICKS(params->period) - high_period;

    // Ensure that the tick periods do not overflow
    if (high_period == pdMS_TO_TICKS(0) || low_period == pdMS_TO_TICKS(0)) {
        // Handle error, for example, set a minimum value
        high_period = pdMS_TO_TICKS(10); // minimum non-zero period
        low_period = pdMS_TO_TICKS(10);
    }

    for (;;) {
        for (int i = 0; i < PIXEL_COUNT; i++) {
            set_pixel_effect_rgb(i, params->red, params->green, params->blue);
        }
        vTaskDelay(high_period);
        for (int i = 0; i < PIXEL_COUNT; i++) {
            set_pixel_effect_release(i);
        }
        vTaskDelay(low_period);
    }
}

void flash(uint8_t red, uint8_t green, uint8_t blue, uint16_t period, float duty) {
    // Dynamically allocate memory for the parameters
    struct FlashParams *params = pvPortMalloc(sizeof(struct FlashParams));
    if (params == NULL) {
        // Handle memory allocation failure (e.g., log it, return, etc.)
        return;
    }

    params->red = red;
    params->green = green;
    params->blue = blue;
    params->period = period;
    params->duty = duty;

    // Create the task and check if it's created successfully
    BaseType_t result = xTaskCreate(task_flash, "TaskFlash", 2048, (void*) params, 1, NULL);
    if (result != pdPASS) {
        // Handle task creation failure (e.g., log it, free memory, etc.)
        vPortFree(params);
    }
}

void task_burst(void *parameters) {
    struct BurstParams *params = (struct BurstParams *) parameters;

    // Ensure duration is a reasonable value (e.g., non-zero and within range)
    if (params->duration == 0) {
        params->duration = 10; // set a minimum duration if 0 is passed
    }

    // Convert duration to ticks
    TickType_t period = pdMS_TO_TICKS(params->duration);

    // Check if duration is too large for TickType_t to handle
    if (period == pdMS_TO_TICKS(0)) {
        // Handle overflow case, adjust to a default value
        period = pdMS_TO_TICKS(10); // default to 10 ms
    }

    for (int i = 0; i < PIXEL_COUNT; i++) {
        set_pixel_effect_rgb(i, params->red, params->green, params->blue);
    }
    vTaskDelay(period);
    for (int i = 0; i < PIXEL_COUNT; i++) {
        set_pixel_effect_release(i);
    }

    vTaskDelete(NULL);
}

void burst(uint8_t red, uint8_t green, uint8_t blue, uint16_t duration) {
    // Dynamically allocate memory for the parameters
    struct BurstParams *params = pvPortMalloc(sizeof(struct BurstParams));
    if (params == NULL) {
        // Handle memory allocation failure (e.g., log it, return, etc.)
        return;
    }

    params->red = red;
    params->green = green;
    params->blue = blue;
    params->duration = duration;

    // Create the task and check if it's created successfully
    BaseType_t result = xTaskCreate(task_burst, "TaskBurst", 2048, (void*) params, 1, NULL);
    if (result != pdPASS) {
        // Handle task creation failure (e.g., log it, free memory, etc.)
        vPortFree(params);
    }
}