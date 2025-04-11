//
// Created by Mathieu Durand on 2025-02-24.
//

#include <stdio.h>
#include <driver/gpio.h>
#include <math.h>
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "power.h"
#include "esp_log.h"

#define SHERPENT_V2

#define CHARGE_STATE_PIN 4
#ifdef SHERPENT_V2
#define STAY_ON_PIN 5
#else
#define STAY_ON_PIN 5
#endif
#define DIVIDER_RATIO (2.0f/3.0f)

#ifdef SHERPENT_V2
#define ADC_CHANNEL ADC_CHANNEL_1
#else
#define ADC_CHANNEL ADC_CHANNEL_4
#endif
#define ADC_ATTEN ADC_ATTEN_DB_12   // 11dB attenuation (0-3.6V range)

static const char *TAG = "POWER";

// ADC Handles
static adc_oneshot_unit_handle_t adc1_handle;
static adc_cali_handle_t adc_cali_handle = NULL;
static bool powered = false;

VoltageSoC voltageSoCTable[] = {
        {4.2f, 100}, {4.15f, 95}, {4.11f, 90}, {4.08f, 85},
        {4.02f, 80}, {3.98f, 75}, {3.95f, 70}, {3.91f, 65},
        {3.87f, 60}, {3.82f, 55}, {3.78f, 50}, {3.74f, 45},
        {3.70f, 40}, {3.66f, 35}, {3.62f, 30}, {3.58f, 25},
        {3.54f, 20}, {3.49f, 15}, {3.42f, 10}, {3.30f, 5},
        {3.00f, 0}
};

// Function prototypes
static bool adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle);

void power_init() {
    static bool initialized = false;
    if (initialized) return;
    initialized = true;

    //------------- ADC1 Init ---------------//
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    //------------- ADC1 Config ---------------//
    adc_oneshot_chan_cfg_t config = {
        .atten = ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL, &config));

    //------------- ADC1 Calibration Init ---------------//
    bool do_calibration = adc_calibration_init(ADC_UNIT_1, ADC_CHANNEL, ADC_ATTEN, &adc_cali_handle);
    
    if (do_calibration) {
        ESP_LOGI(TAG, "ADC Calibration Initialized");
    } else {
        ESP_LOGW(TAG, "ADC Calibration Not Supported");
    }

    gpio_reset_pin(STAY_ON_PIN);
    gpio_set_direction(STAY_ON_PIN, GPIO_MODE_OUTPUT);
}

float get_battery_voltage() {
    int adc_raw;
    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC_CHANNEL, &adc_raw));

    int voltage_mv = 0;
    if (adc_cali_handle) {
        ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc_cali_handle, adc_raw, &voltage_mv));
    }

    return voltage_mv / (1000.0f * DIVIDER_RATIO);  // Convert to volts
}

//------------- ADC Calibration Functions ---------------//
static bool adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle) {
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;
    bool calibrated = false;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    if (!calibrated) {
        ESP_LOGI(TAG, "Calibration Scheme: Curve Fitting");
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = unit,
            .chan = channel,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    if (!calibrated) {
        ESP_LOGI(TAG, "Calibration Scheme: Line Fitting");
        adc_cali_line_fitting_config_t cali_config = {
            .unit_id = unit,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

    *out_handle = handle;
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "ADC Calibration Success");
    } else {
        ESP_LOGW(TAG, "ADC Calibration Not Supported");
    }
    return calibrated;
}

bool is_below_min_voltage(float voltage) {
    return voltage < BATTERY_VOLTAGE_MIN;
}
bool is_above_max_voltage(float voltage) {
    return voltage > BATTERY_VOLTAGE_MAX;
}
bool is_below_max_critical_voltage(float voltage) {
    return voltage > BATTERY_VOLTAGE_MAX_CRITICAL;
}

bool is_battery_voltage_safe(float voltage) {
    return !is_below_min_voltage(voltage) && !is_above_max_voltage(voltage);
}

float get_battery_percentage() {
    float voltage = get_battery_voltage();
    int tableSize = sizeof(voltageSoCTable) / sizeof(voltageSoCTable[0]);

    // If the voltage is below the minimum, return 0%
    if (voltage <= voltageSoCTable[tableSize - 1].voltage) {
        return 0.0f;
    }
    // If the voltage is above the maximum, return 100%
    if (voltage >= voltageSoCTable[0].voltage) {
        return 1.0f;
    }

    // Find the range where the voltage falls
    for (int i = 0; i < tableSize - 1; i++) {
        if (voltage <= voltageSoCTable[i].voltage && voltage > voltageSoCTable[i + 1].voltage) {
            float voltageRange = voltageSoCTable[i].voltage - voltageSoCTable[i + 1].voltage;
            float socRange = voltageSoCTable[i].soc - voltageSoCTable[i + 1].soc;
            float voltageOffset = voltage - voltageSoCTable[i + 1].voltage;
            return (voltageSoCTable[i + 1].soc + (voltageOffset / voltageRange) * socRange) / 100.0f;
        }
    }

    return 0.0f; // Fallback case, shouldn't reach here
}

bool is_battery_charging() {
    return get_battery_voltage() > 4.2f;
}

void set_powered(bool powered_) {
    gpio_set_level(STAY_ON_PIN, powered_ ? 1 : 0);
    powered = powered_;
}

bool is_powered() {
    return powered;
}