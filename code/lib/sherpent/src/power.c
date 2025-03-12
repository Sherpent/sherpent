//
// Created by Mathieu Durand on 2025-02-24.
//

#include <driver/adc.h>
#include "power.h"

#define CHARGE_STATE_PIN 4
#define STAY_ON_PIN 5

#define ADC_WIDTH ADC_WIDTH_BIT_12  // 12-bit ADC resolution (0-4095)
#define ADC_PIN ADC1_CHANNEL_4     // Example: ADC1 Channel 4 corresponds to GPIO20 (check your ESP32 variant)

#define ADC_TO_VOLTAGE ((3.3f / 4095.0f) * (3.0f / 2.0f))
#define ADC_TO_PERCENTAGE ((1.0f / 4095.0f) * (3.0f / 2.0f))

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