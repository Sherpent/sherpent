//
// Created by Mathieu Durand on 2025-02-24.
//

#include <soc/io_mux_reg.h>
#include <driver/gpio.h>
#include <neopixel.h>
#include <string.h>
#include <math.h>
#include <driver/uart.h>
#include "led.h"
#include "esp_log.h"

static tNeopixelContext neopixel = NULL;

static uint32_t main_color[PIXEL_COUNT] = {0};
static struct xLIST effect_queue[PIXEL_COUNT];
static TaskHandle_t led_task = NULL;

void led_init() {
    static bool initialized = false;
    if (initialized) return;
    initialized = true;

    for (int i = 0; i < PIXEL_COUNT; i++) {
        vListInitialise(&effect_queue[i]);
    }
    xTaskCreate(task_led, "LEDTask", 2048, NULL, 1, &led_task);

    uart_driver_delete(UART_NUM_0);
    gpio_set_direction(NEOPIXEL_PIN, GPIO_MODE_INPUT_OUTPUT); // Make sure it's set as GPIO and not as UART
    neopixel = neopixel_Init(PIXEL_COUNT, NEOPIXEL_PIN);
    xTaskNotifyGive(led_task); // Update LEDs
}


uint32_t gamma_correct_rgb(uint32_t color) {
    uint8_t red = powf((float) ((color >> 16) & 0xFF) / 255.0f, GAMMA_RED) * 255;
    uint8_t green = powf((float) ((color >> 8) & 0xFF) / 255.0f, GAMMA_GREEN) * 255;
    uint8_t blue = powf((float) (color & 0xFF) / 255.0f, GAMMA_BLUE) * 255;

    return NP_RGB(red, green, blue);
}

void set_pixel_rgb(uint32_t pixel_num, uint8_t red, uint8_t green, uint8_t blue) {
    if (pixel_num >= PIXEL_COUNT) {
        ESP_LOGW("LED", "Trying to set color for invalid pixel index, %d, Valid pixel index are in the range [0, %d]", (int) pixel_num, PIXEL_COUNT - 1);
        return;
    }

    main_color[pixel_num] = NP_RGB(red, green, blue);
    xTaskNotifyGive(led_task);
}

void set_pixel_gradient(enum gradient_type_t type, uint8_t start_red, uint8_t start_green, uint8_t start_blue, uint8_t end_red, uint8_t end_green, uint8_t end_blue) {
    // SPIRAL : 3 6 5 4 2 7 9 1 0 8
    // LONGITUDINAL : 3 2 0 6 7 8 4 1 5 9

    //3 2 0
    //6 7 8

    // 4 1
    // 5 9

    static const uint8_t table[3][PIXEL_COUNT] = {
            [SIMPLE] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9},
            [LONGITUDINAL] = {3, 2, 0, 6, 7, 8, 4,1, 5, 9},
            [SPIRAL] = {3, 6, 5, 4, 2, 7, 9, 1, 0, 8}
    };

    for (int i = 0; i < PIXEL_COUNT; i++) {
        float t = (float) i / (PIXEL_COUNT - 1);
        uint8_t red = (uint8_t) ((float) start_red * (1.0f - t) + (float) end_red * t);
        uint8_t green = (uint8_t) ((float) start_green * (1.0f - t) + (float) end_green * t);
        uint8_t blue = (uint8_t) ((float) start_blue * (1.0f - t) + (float) end_blue * t);
        set_pixel_rgb(i, red, green, blue);
    }
}

struct xLIST_ITEM *set_pixel_effect_rgb(uint32_t pixel_num, uint8_t red, uint8_t green, uint8_t blue) {
    return set_pixel_effect_argb(pixel_num, 255, red, green, blue);
}

struct xLIST_ITEM *set_pixel_effect_argb(uint32_t pixel_num, uint8_t alpha, uint8_t red, uint8_t green, uint8_t blue) {
    if (pixel_num >= PIXEL_COUNT) {
        ESP_LOGW("LED", "Invalid pixel index: %d (Valid range: 0-%d)", (int) pixel_num, PIXEL_COUNT - 1);
        return NULL;
    }

    // Allocate memory for a new effect item
    effect_item_t *item = pvPortMalloc(sizeof(effect_item_t));
    if (item == NULL) {
        ESP_LOGE("LED", "Memory allocation failed for effect item!");
        return NULL;
    }

    // Set color value
    item->color = NP_ARGB(alpha, red, green, blue);

    // Initialize and set list item
    vListInitialiseItem(&item->listItem);
    listSET_LIST_ITEM_OWNER(&item->listItem, item);

    // Ensure the queue is initialized before inserting
    if (listLIST_IS_INITIALISED(&effect_queue[pixel_num])) {
        vListInsertEnd(&effect_queue[pixel_num], &item->listItem);
    } else {
        ESP_LOGE("LED", "Effect queue for pixel %d is not initialized!", (int) pixel_num);
        vPortFree(item);
        return NULL;
    }

    // Notify the LED task if it exists
    if (led_task != NULL) {
        xTaskNotifyGive(led_task);
    } else {
        ESP_LOGW("LED", "LED task is NULL; notification not sent.");
    }

    return &item->listItem;
}

void set_pixel_effect_release(uint32_t pixel_num, struct xLIST_ITEM *handle, bool silently) {
    if (pixel_num >= PIXEL_COUNT) {
        ESP_LOGW("LED", "Trying to release effect color for invalid pixel index, %d, Valid pixel index are in the range [0, %d]", (int) pixel_num, PIXEL_COUNT - 1);
        return;
    }

    effect_item_t *effect = (effect_item_t *) listGET_LIST_ITEM_OWNER(handle);
    uxListRemove(handle);
    free(effect);

    if (!silently) {
        xTaskNotifyGive(led_task);
    }
}

void stop_effect(TaskHandle_t effect_handle) {
    xTaskNotifyGive(effect_handle);
}

static uint32_t blend_colors(uint32_t main_color, uint32_t effect_color) {
    uint8_t alpha = (effect_color >> 24) & 0xFF;
    uint8_t inverse_alpha = 255 - alpha;

    uint8_t red   = (((main_color >> 16) & 0xFF) * inverse_alpha + ((effect_color >> 16) & 0xFF) * alpha) >> 8;
    uint8_t green = (((main_color >> 8) & 0xFF) * inverse_alpha + ((effect_color >> 8) & 0xFF) * alpha) >> 8;
    uint8_t blue  = ((main_color & 0xFF) * inverse_alpha + (effect_color & 0xFF) * alpha) >> 8;

    return NP_RGB(red, green, blue);
}


void task_led(void *parameters) {
    uint32_t color;

    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        for (int i = 0; i < PIXEL_COUNT; i++) {
            if (!listLIST_IS_EMPTY(&effect_queue[i])) {  // Corrected condition

                ListItem_t *item = listGET_HEAD_ENTRY(&effect_queue[i]);
                effect_item_t *effect = (effect_item_t *) listGET_LIST_ITEM_OWNER(item);
                color = blend_colors(main_color[i], effect->color);  // Cleaner blending logic
            } else {
                color = main_color[i];
            }

            color = gamma_correct_rgb(color);
            tNeopixel pixel = {i, color};
            if (!neopixel_SetPixel(neopixel, &pixel, 1)) {
                ESP_LOGE("LED", "Failed to control LEDs");
            }
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

    ListItem_t *handles[PIXEL_COUNT];

    for (;;) {
        if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(0)) > 0) break;
        for (int i = 0; i < PIXEL_COUNT; i++) {
            handles[i] = set_pixel_effect_rgb(i, params->red, params->green, params->blue);
        }
        vTaskDelay(high_period);
        for (int i = 0; i < PIXEL_COUNT; i++) {
            set_pixel_effect_release(i, handles[i], false);
        }
        if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(0)) > 0) break;
        vTaskDelay(low_period);
    }

    vTaskDelete(NULL);
}

TaskHandle_t flash(uint8_t red, uint8_t green, uint8_t blue, uint16_t period, float duty) {
    // Dynamically allocate memory for the parameters
    struct FlashParams *params = pvPortMalloc(sizeof(struct FlashParams));

    params->red = red;
    params->green = green;
    params->blue = blue;
    params->period = period;
    params->duty = duty;

    // Create the task and check if it's created successfully
    TaskHandle_t handle = NULL;
    BaseType_t result = xTaskCreate(task_flash, "TaskFlash", 2048, (void*) params, 1, &handle);
    if (result != pdPASS) {
        // Handle task creation failure (e.g., log it, free memory, etc.)
        vPortFree(params);
    }
    return handle;
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

    ListItem_t *handles[PIXEL_COUNT];
    for (int i = 0; i < PIXEL_COUNT; i++) {
        handles[i] = set_pixel_effect_rgb(i, params->red, params->green, params->blue);
    }
    vTaskDelay(period);
    for (int i = 0; i < PIXEL_COUNT; i++) {
        set_pixel_effect_release(i, handles[i], false);
    }

    vTaskDelete(NULL);
}

TaskHandle_t burst(uint8_t red, uint8_t green, uint8_t blue, uint16_t duration) {
    // Dynamically allocate memory for the parameters
    struct BurstParams *params = pvPortMalloc(sizeof(struct BurstParams));

    params->red = red;
    params->green = green;
    params->blue = blue;
    params->duration = duration;

    // Create the task and check if it's created successfully
    TaskHandle_t handle = NULL;
    BaseType_t result = xTaskCreate(task_burst, "TaskBurst", 2048, (void*) params, 1, &handle);
    if (result != pdPASS) {
        // Handle task creation failure (e.g., log it, free memory, etc.)
        vPortFree(params);
    }
    return handle;
}

void task_breath(void *parameters) {
    struct BreathParams *params = (struct BreathParams *) parameters;
    ListItem_t *handles[PIXEL_COUNT];

    TickType_t lastWakeTime = xTaskGetTickCount();
    const TickType_t delay = pdMS_TO_TICKS(10);

    for (;;) {
        memset(handles, 0, sizeof(handles));  // Clear handles array

        uint64_t time = pdTICKS_TO_MS(xTaskGetTickCount());
        double phase = (double) time / (double) params->period;  // Correct phase calculation
        uint8_t alpha = (uint8_t)((sin(M_TWOPI * phase) * 0.5 + 0.5) * (params->alpha_high - params->alpha_low) + params->alpha_low);

        // Apply effect
        if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(0)) > 0) break;
        for (int i = 0; i < PIXEL_COUNT; i++) {
            handles[i] = set_pixel_effect_argb(i, alpha, params->red, params->green, params->blue);
        }

        vTaskDelayUntil(&lastWakeTime, delay);  // Maintain precise timing

        // Release effect
        for (int i = 0; i < PIXEL_COUNT; i++) {
            if (handles[i] != NULL) {  // Ensure we only release valid handles
                set_pixel_effect_release(i, handles[i], true);
            }
        }
        if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(0)) > 0) break;
    }

    vTaskDelete(NULL);
}

TaskHandle_t breath(uint8_t red, uint8_t green, uint8_t blue,  uint8_t alpha_low, uint8_t alpha_high, uint16_t period) {
    // Dynamically allocate memory for the parameters
    struct BreathParams *params = pvPortMalloc(sizeof(struct BreathParams));

    params->red = red;
    params->green = green;
    params->blue = blue;
    params->alpha_low = alpha_low;
    params->alpha_high = alpha_high;
    params->period = period;

    // Create the task and check if it's created successfully
    TaskHandle_t handle = NULL;
    BaseType_t result = xTaskCreate(task_breath, "TaskBreath", 2048, (void*) params, 1, &handle);
    if (result != pdPASS) {
        // Handle task creation failure (e.g., log it, free memory, etc.)
        vPortFree(params);
    }
    return handle;
}