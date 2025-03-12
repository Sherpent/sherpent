#include "safety.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"

void safety_init() {
    static bool initialized = false;
    if (initialized) return;
    initialized = true;

    power_init();
    vSafetyCheckerTask(); // Calling immediately for safety
    xTaskCreate(vSafetyCheckerTask, "Safety Checker", 4096, NULL, 1, NULL);
}

void vSafetyCheckerTask() {
    float voltage = get_battery_voltage();
    bool battery_safe = is_battery_voltage_safe(voltage);

    if (!battery_safe) {
        if (is_below_min_voltage(voltage)) {
            ESP_LOGE("SAFETY", "Battery voltage is low %.2lf V", voltage);
            ESP_LOGE("SAFETY", "----------------- SHUTTING DOWN -----------------");
            set_powered(false);
        }

        if (is_above_max_voltage(voltage)) {
            ESP_LOGW("SAFETY", "Battery voltage is high %.2lf V", voltage);
        }

        if (is_below_min_voltage(voltage)) {
            ESP_LOGE("SAFETY", "Battery voltage is critical %.2lf V", voltage);
            ESP_LOGE("SAFETY", "----------------- SHUTTING DOWN -----------------");
            set_powered(false);
        }
    }

    vTaskDelay(1000);
}