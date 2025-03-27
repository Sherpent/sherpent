//
// Created by Mathieu Durand on 2025-03-27.
//

#ifndef HEAD_MOVEMENT_H
#define HEAD_MOVEMENT_H

#define PI 3.141592653589793f
#define TWO_PI (2.0f * PI)

void set_turn_angle(float angle);
void set_raise_angle(float angle);
void set_slither_frequency(float frequency);

void init_slither_task();

#endif //HEAD_MOVEMENT_H
