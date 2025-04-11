//
// Created by Mathieu Durand on 2025-02-24.
//

#ifndef LED_H
#define LED_H

#include <stdbool.h>
#include <stdint-gcc.h>
#include <freertos/FreeRTOS.h>
#include <freertos/list.h>

#define COLOR_QUEUE_SIZE 5
#define PIXEL_COUNT 10

#define NEOPIXEL_PIN 0
#define LED_BUILTIN_PIN 10
#define BUTTON_PIN 3

#define NP_ARGB(a, r, g, b)   ( ((uint32_t)(a) & 0xFF) << 24 \
                        | ((uint32_t)(r) & 0xFF) << 16   \
                        | ((uint32_t)(g) & 0xFF) << 8   \
                        | ((uint32_t)(b) & 0xFF) )

typedef struct {
    uint32_t color;            // Your data
    ListItem_t listItem;  // Required FreeRTOS list item
} effect_item_t;

void led_init();

/**
 * Sets the pixel at pixel_num to color.
 */
void set_pixel_rgb(uint32_t pixel_num, uint8_t red, uint8_t green, uint8_t blue);

struct xLIST_ITEM *set_pixel_effect_rgb(uint32_t pixel_num, uint8_t red, uint8_t green, uint8_t blue);
struct xLIST_ITEM *set_pixel_effect_argb(uint32_t pixel_num, uint8_t alpha, uint8_t red, uint8_t green, uint8_t blue);
void set_pixel_effect_release(uint32_t pixel_num, struct xLIST_ITEM *handle, bool silently);

void stop_effect(TaskHandle_t effect_handle);

void task_led(void *parameters);

struct FlashParams {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint16_t period;
    float duty;
};
/**
 * Flash for a set period of time with a set color
 * @param parameters (red: uint8_t, green: uint8_t, blue: uint8_t, period: uint16_t, duty: float)
 */
void task_flash(void *parameters);
TaskHandle_t flash(uint8_t red, uint8_t green, uint8_t blue, uint16_t period, float duty);

struct BurstParams {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint16_t duration;
};
/**
 * Flash for a set period of time with a set color once
 * @param parameters (red: uint8_t, green: uint8_t, blue: uint8_t, duration: uint16_t)
 */
void task_burst(void *parameters);
TaskHandle_t burst(uint8_t red, uint8_t green, uint8_t blue, uint16_t duration);

struct BreathParams {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint8_t alpha_low;
    uint8_t alpha_high;
    uint16_t period;
};

void task_breath(void *parameters);
TaskHandle_t breath(uint8_t red, uint8_t green, uint8_t blue,  uint8_t alpha_low, uint8_t alpha_high, uint16_t period);

#endif // LED_H
