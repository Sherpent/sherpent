//
// Created by Mathieu Durand on 2025-02-24.
//

#include <soc/io_mux_reg.h>
#include <neopixel.h>
#include <freertos/FreeRTOS.h>
#include "led.h"
#include "esp_log.h"

tNeopixelContext neopixel = NULL;

void led_init() {
    neopixel = neopixel_Init(PIXEL_COUNT, NEOPIXEL_PIN);
    //PIN_FUNC_SELECT(NEOPIXEL_PIN, PIN_FUNC_GPIO);
    //gpio_set_direction(NEOPIXEL_PIN, GPIO_MODE_OUTPUT);
    //gpio_matrix_out(NEOPIXEL_PIN, CPU_GPIO_OUT5_IDX, false, false);

    for (int i = 0; i < PIXEL_COUNT; i++) {
        set_pixel_rgb(i, 0, 0, 0);
    }
}

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

// Declare a global mutex (or semaphore) for synchronizing access to neopixel
static SemaphoreHandle_t neopixel_mutex = NULL;

bool set_pixel_rgb(uint32_t pixel_num, uint8_t red, uint8_t green, uint8_t blue) {
    // Initialize the mutex if not already initialized
    if (neopixel_mutex == NULL) {
        neopixel_mutex = xSemaphoreCreateMutex();
        if (neopixel_mutex == NULL) {
            ESP_LOGW("LED", "Failed to create mutex");
            return false;
        }
    }

    // Try to take the mutex (blocking if needed)
    if (xSemaphoreTake(neopixel_mutex, portMAX_DELAY) != pdPASS) {
        ESP_LOGW("LED", "Failed to acquire mutex");
        return false;
    }

    // Critical section: Access shared resource (neopixel)
    if (neopixel == NULL) {
        ESP_LOGW("LED", "neopixel == NULL, make sure to call led_init()");
        xSemaphoreGive(neopixel_mutex); // Release the mutex before returning
        return false;
    }

    tNeopixel pixel = { pixel_num, NP_RGB(red, green, blue) };
    bool result = neopixel_SetPixel(neopixel, &pixel, 1);

    // Release the mutex after operation
    xSemaphoreGive(neopixel_mutex);

    return result;
}

/**
 * Flash for a set period of time with a set color
 * @param parameters (red: uint8_t, green: uint8_t, blue: uint8_t, period: uint16_t, duty: float)
 */
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
            set_pixel_rgb(i, params->red, params->green, params->blue);
        }
        vTaskDelay(high_period);
        for (int i = 0; i < PIXEL_COUNT; i++) {
            set_pixel_rgb(i, 0, 0, 0);
        }
        vTaskDelay(low_period);
    }
}

/**
 * Flash for a set period of time with a set color once
 * @param parameters (red: uint8_t, green: uint8_t, blue: uint8_t, duration: uint16_t)
 */
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
        set_pixel_rgb(i, params->red, params->green, params->blue);
    }
    vTaskDelay(period);
    for (int i = 0; i < PIXEL_COUNT; i++) {
        set_pixel_rgb(i, 0, 0, 0);
    }

    vTaskDelete(NULL);
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