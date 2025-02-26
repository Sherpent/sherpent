//
// Created by Mathieu Durand on 2025-02-24.
//

#ifndef LED_H
#define LED_H

#include <stdbool.h>
#include <stdint-gcc.h>

#define PIXEL_COUNT 10

#define NEOPIXEL_PIN 21
#define LED_BUILTIN_PIN 10
#define BUTTON_PIN 3

void led_init();

/**
 * Sets the pixel at pixel_num to color.
 */
bool set_pixel_rgb(uint32_t pixel_num, uint8_t red, uint8_t green, uint8_t blue);

struct FlashParams {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint16_t period;
    float duty;
};

struct BurstParams {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint16_t duration;
};

/**
 * Flash for a set period of time with a set color
 * @param parameters (red: uint8_t, green: uint8_t, blue: uint8_t, period: uint16_t, duty: float)
 */
void task_flash(void *parameters);

/**
 * Flash for a set period of time with a set color once
 * @param parameters (red: uint8_t, green: uint8_t, blue: uint8_t, duration: uint16_t)
 */
void task_burst(void *parameters);

void flash(uint8_t red, uint8_t green, uint8_t blue, uint16_t period, float duty);
void burst(uint8_t red, uint8_t green, uint8_t blue, uint16_t duration);

#endif // LED_H