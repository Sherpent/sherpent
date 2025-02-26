//
// Created by Mathieu Durand on 2025-02-24.
//

#ifndef LED_H
#define LED_H

#include <stdbool.h>
#include <stdint-gcc.h>

#define COLOR_QUEUE_SIZE 5
#define PIXEL_COUNT 10

#define NEOPIXEL_PIN 21
#define LED_BUILTIN_PIN 10
#define BUTTON_PIN 3

void led_init();

/**
 * Sets the pixel at pixel_num to color.
 */
void set_pixel_rgb(uint32_t pixel_num, uint8_t red, uint8_t green, uint8_t blue);

void set_pixel_effect_rgb(uint32_t pixel_num, uint8_t red, uint8_t green, uint8_t blue);
void set_pixel_effect_release(uint32_t pixel_num);

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
void flash(uint8_t red, uint8_t green, uint8_t blue, uint16_t period, float duty);

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
void burst(uint8_t red, uint8_t green, uint8_t blue, uint16_t duration);

#endif // LED_H