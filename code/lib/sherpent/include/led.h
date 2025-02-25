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

#endif // LED_H