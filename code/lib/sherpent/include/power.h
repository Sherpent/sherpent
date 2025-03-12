//
// Created by Mathieu Durand on 2025-02-24.
//

#ifndef POWER_H
#define POWER_H

#include <stdbool.h>

#define BATTERY_VOLTAGE_MIN 3.0f   // Example minimum voltage (3.0V)
#define BATTERY_VOLTAGE_MAX 4.2f   // Example maximum voltage (4.2V)

void power_init();

float get_battery_voltage();
float get_battery_percentage();

bool is_battery_charging();

void set_powered(bool powered);

bool is_powered();

#endif //POWER_H
