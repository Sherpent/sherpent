//
// Created by Mathieu Durand on 2025-02-24.
//

#ifndef SERVO_H
#define SERVO_H

#define SERVO_PITCH_PIN 6
#define SERVO_YAW_PIN 7

enum servo_type_t {
    PITCH,
    YAW,
};

void servo_init();

void set_servo_angle(enum servo_type_t servo_type, float angle);

#endif //SERVO_H
