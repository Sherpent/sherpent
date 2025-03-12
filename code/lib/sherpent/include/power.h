//
// Created by Mathieu Durand on 2025-02-24.
//

#ifndef POWER_H
#define POWER_H

#include <stdbool.h>

#define BATTERY_VOLTAGE_MIN 2.8f   // Example minimum voltage (2.8V)
#define BATTERY_VOLTAGE_MAX 4.5f   // Example maximum voltage (4.2V)
#define BATTERY_VOLTAGE_MAX_CRITICAL 6.0f   // Example maximum voltage (6.0V)

void power_init();

float get_battery_voltage();
bool is_below_min_voltage(float voltage);
bool is_above_max_voltage(float voltage);
bool is_below_max_critical_voltage(float voltage);
bool is_battery_voltage_safe(float voltage);

float get_battery_percentage();

bool is_battery_charging();

void set_powered(bool powered);

bool is_powered();

#endif //POWER_H
