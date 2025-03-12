//
// Created by Mathieu Durand on 2025-02-24.
//

#ifndef POWER_H
#define POWER_H

#include <stdbool.h>

#define CHARGE_STATE_PIN 4
#define STAY_ON_PIN 5

#define ADC_WIDTH ADC_WIDTH_BIT_12  // 12-bit ADC resolution (0-4095)
#define ADC_PIN ADC1_CHANNEL_4     // Example: ADC1 Channel 4 corresponds to GPIO20 (check your ESP32 variant)

#define ADC_TO_VOLTAGE ((3.3f / 4095.0f) * (3.0f / 2.0f))
#define ADC_TO_PERCENTAGE ((1.0f / 4095.0f) * (3.0f / 2.0f))

void power_init();

float get_battery_voltage();
float get_battery_percentage();

bool is_battery_charging();

void set_powered(bool powered);

bool is_powered();

#endif //POWER_H
