//
// Created by Mathieu Durand on 2025-02-24.
//

#include <soc/io_mux_reg.h>
#include <neopixel.h>
#include "led.h"
#include "esp_log.h"

tNeopixelContext neopixel = NULL;

void led_init() {
    neopixel = neopixel_Init(PIXEL_COUNT, NEOPIXEL_PIN);
    //PIN_FUNC_SELECT(NEOPIXEL_PIN, PIN_FUNC_GPIO);
    //gpio_set_direction(NEOPIXEL_PIN, GPIO_MODE_OUTPUT);
    //gpio_matrix_out(NEOPIXEL_PIN, CPU_GPIO_OUT5_IDX, false, false);
}

bool set_pixel_rgb(uint32_t pixel_num, uint8_t red, uint8_t green, uint8_t blue) {
    if (neopixel == NULL) {
        ESP_LOGW("LED", "neopixel == NULL, make sure to call led_init()");
        return false;
    }

    tNeopixel pixel = { pixel_num, NP_RGB(red, green,  blue) };
    return neopixel_SetPixel(neopixel, &pixel, 1);
    //return led_strip_set_pixel_rgb(&led_strip, pixel_num, red, green, blue);
}