//
// Created by Mathieu Durand on 2025-03-27.
//

#include <stdint.h>
#include <servo.h>

#ifndef HEAD_MOVEMENT_H
#define HEAD_MOVEMENT_H

#define PI 3.141592653589793f
#define TWO_PI (2.0f * PI)

int8_t get_angle(enum servo_type_t axis, uint8_t segment_id);

void set_axis(enum servo_type_t axis, uint8_t segment_id, int8_t angle);

void set_turn_angle(float angle);
void set_raise_angle(float angle);
void set_slither_frequency(float frequency);
void set_sidewinding(float sidewinding_);

void roll(float roll_angle, float tightness);

void init_slither_task();

#endif //HEAD_MOVEMENT_H
