//
// Created by Mathieu Durand on 2025-03-27.
//

#include <stdint.h>
#include <servo.h>
#include <stdbool.h>

#ifndef HEAD_MOVEMENT_H
#define HEAD_MOVEMENT_H
#define MAX_FORWARD_SLITHER_FREQUENCY 0.5f
#define MAX_TURNING_SLITHER_FREQUENCY 1.0f
#define AMPLITUDE_DAMPENING 0.9f
#define MAX_FORWARD_SLITHER_AMPLITUDE 1.0f
#define MAX_TURNING_SLITHER_AMPLITUDE 0.5f

#define MOUTH_OPENED_ANGLE 0.0f
#define MOUTH_CLOSED_ANGLE (-35.0f)

#define PI 3.141592653589793f
#define TWO_PI (2.0f * PI)

typedef struct {
    uint8_t segment_id;
    bool pitch;
    bool yaw;
    int8_t pitch_angle;
    int8_t yaw_angle;
} angle_queue_item_t;

int8_t get_angle(enum servo_type_t axis, uint8_t segment_id);

void set_both_axis(uint8_t segment_id, int8_t pitch_angle, int8_t yaw_angle);
void set_axis(enum servo_type_t axis, uint8_t segment_id, int8_t angle);

void set_both_axis_direct(uint8_t segment_id, int8_t pitch_angle, int8_t yaw_angle);
void set_axis_direct(enum servo_type_t axis, uint8_t segment_id, int8_t angle);

void control_raw(float x1_, float y, float x2_, float mouth);
void set_turn_angle(float angle);
void set_raise_angle(float angle);
void set_slither_frequency(float frequency);
void set_slither_amplitude(float amplitude);
void set_sidewinding(float sidewinding_);

void look_up(float look_pitch, float look_yaw);

void roll(float roll_angle, float tightness);

void init_movement();
void init_slither_task();

#endif //HEAD_MOVEMENT_H
