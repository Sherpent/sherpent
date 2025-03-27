//
// Created by Mathieu Durand on 2025-02-24.
//

#include "servo.h"
#include "driver/ledc.h"
#include "esp_log.h"

#define PWM_FREQ_HZ 50    // 50Hz for servo motors
#define PWM_RESOLUTION 10  // 8-bit resolution (0-255)

#define MIN_DUTY 350.0f
#define MAX_DUTY 1040.0f
#define MIN_ANGLE (-90.0f)
#define MAX_ANGLE 90.0f

static ledc_timer_config_t ledc_timer = {
        .duty_resolution = PWM_RESOLUTION,     // Resolution of PWM duty
        .freq_hz = PWM_FREQ_HZ,                // Frequency of PWM signal
        .speed_mode = LEDC_LOW_SPEED_MODE,     // LEDC low-speed mode
        .timer_num = LEDC_TIMER_0              // Timer 0
};

static ledc_channel_config_t ledc_channel[] = {
        [PITCH] = {
                .channel    = LEDC_CHANNEL_0,
                .duty       = 0,                        // Initial duty cycle (start at 0)
                .gpio_num   = SERVO_PITCH_PIN,          // GPIO pin to use
                .speed_mode = LEDC_LOW_SPEED_MODE,      // Speed mode
                .timer_sel  = LEDC_TIMER_0              // Select timer
        },
        [YAW] = {
                .channel    = LEDC_CHANNEL_1,
                .duty       = 0,                        // Initial duty cycle (start at 0)
                .gpio_num   = SERVO_YAW_PIN,            // GPIO pin to use
                .speed_mode = LEDC_LOW_SPEED_MODE,      // Speed mode
                .timer_sel  = LEDC_TIMER_0              // Select timer
        }
};

void servo_init() {
    static bool initialized = false;
    if (initialized) return;
    initialized = true;

    // Reset and configure GPIOs after LEDC is set up
    //gpio_reset_pin(SERVO_PITCH_PIN);
    //gpio_reset_pin(SERVO_YAW_PIN);
    //gpio_set_direction(SERVO_PITCH_PIN, GPIO_MODE_OUTPUT);

    // Ensure GPIO starts low before LEDC takes control
    //gpio_set_level(SERVO_PITCH_PIN, 0);
    // gpio_set_level(SERVO_YAW_PIN, 0);

    // Configure LEDC first to avoid unintended GPIO states
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel[PITCH]));
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel[YAW]));

    // Explicitly set LEDC duty cycle to 0 to prevent spikes
    set_servo_angle(YAW, 0);
    set_servo_angle(PITCH, 0);
}

void set_servo_angle(enum servo_type_t servo_type, float angle) {
    if (angle < MIN_ANGLE) angle = MIN_ANGLE;
    if (angle > MAX_ANGLE) angle = MAX_ANGLE;

    float duty = ((angle - MIN_ANGLE) * (0.125f - 0.025f) / (MAX_ANGLE - MIN_ANGLE) + 0.025f);

    ledc_set_duty(ledc_channel[servo_type].speed_mode, ledc_channel[servo_type].channel, (uint32_t) (duty * 1023));
    ledc_update_duty(ledc_channel[servo_type].speed_mode, ledc_channel[servo_type].channel);
}