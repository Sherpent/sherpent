//
// Created by Mathieu Durand on 2025-02-24.
//

#include <driver/adc.h>
#include "power.h"

static bool _powered = false;

void power_init() {
    // ADC width (12-bit resolution is used here)
    adc1_config_width(ADC_WIDTH);

    // Configure the ADC pin (GPIO20 in this case)
    adc1_config_channel_atten(ADC_PIN, ADC_ATTEN_DB_0);  // Use ADC width 0dB attenuation

    gpio_reset_pin(STAY_ON_PIN);
    gpio_set_direction(STAY_ON_PIN, GPIO_MODE_OUTPUT);
}

float get_battery_voltage() {
    return (float) adc1_get_raw(ADC_PIN) * ADC_TO_VOLTAGE;
}
float get_battery_percentage() {
    return (float) adc1_get_raw(ADC_PIN) * ADC_TO_PERCENTAGE;
}

bool is_battery_charging() {
    return false;
}

void set_powered(bool powered) {
    gpio_set_level(STAY_ON_PIN, powered ? 1 : 0);
    _powered = powered;
}

bool is_powered() {
    return _powered;
}