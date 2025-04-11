#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>

// Message ID Enum
enum MessageID: uint8_t {
    LOG,
    REGISTER,
    REGISTER_MASTER,
    INFO_BATTERY,

    SET_YAW,
    SET_PITCH,
    SET_LIGHT,

    CONTROL,
    INFO_YAW,
    INFO_PITCH,
    INFO_BATTERY_MASTER,

    SET_PITCH_YAW,
    INFO_PITCH_YAW,
    SET_LIGHT_GRADIENT,
    INFO_ALL_PITCH_YAW,
};

// General Message Structure
struct Message {
    uint8_t msg_size;
    enum MessageID msg_id;
    uint8_t data[];
};

// Log Level Enum
enum LogLevel: uint8_t {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
};

struct Log {
    uint8_t msg_size;
    enum MessageID msg_id;
    enum LogLevel log_level;
    char message[];
};

struct Register {
    uint8_t msg_size;
    enum MessageID msg_id;
    uint8_t segment_id;
};

struct RegisterMaster {
    uint8_t msg_size;
    enum MessageID msg_id;
};


struct InfoBattery {
    uint8_t msg_size;
    enum MessageID msg_id;
    uint8_t level;
};

struct InfoBatteryMaster {
    uint8_t msg_size;
    enum MessageID msg_id;
    uint8_t segment_id;
    uint8_t level;
};

struct SetPitch {
    uint8_t msg_size;
    enum MessageID msg_id;
    int8_t angle_degrees;
};

struct SetYaw {
    uint8_t msg_size;
    enum MessageID msg_id;
    int8_t angle_degrees;
};

struct SetPitchYaw {
    uint8_t msg_size;
    enum MessageID msg_id;
    int8_t pitch_degrees;
    int8_t yaw_degrees;
};

struct SetLight {
    uint8_t msg_size;
    enum MessageID msg_id;
    uint8_t red;
    uint8_t green;
    uint8_t blue;
};

struct SetLightGradient {
    uint8_t msg_size;
    enum MessageID msg_id;
    uint8_t type;
    uint8_t start_red;
    uint8_t start_green;
    uint8_t start_blue;

    uint8_t end_red;
    uint8_t end_green;
    uint8_t end_blue;
};

// MASTER ONLY
struct Control {
    uint8_t msg_size;
    enum MessageID msg_id;
    float x1;
    float y;
    float x2;
    float mouth;
};

struct InfoYaw {
    uint8_t msg_size;
    enum MessageID msg_id;
    uint8_t segment_id;
    int8_t angle_degrees;
};

struct InfoPitch {
    uint8_t msg_size;
    enum MessageID msg_id;
    uint8_t segment_id;
    int8_t angle_degrees;
};

struct InfoPitchYaw {
    uint8_t msg_size;
    enum MessageID msg_id;
    uint8_t segment_id;
    int8_t pitch_degrees;
    int8_t yaw_degrees;
};

struct InfoAllPitchYaw {
    uint8_t msg_size;
    enum MessageID msg_id;
    int8_t angles[][2];
};

// Function Declarations
//struct Message* packageMessage(enum MessageID message_id, void* data, size_t data_size);
//void* getMessageData(struct Message *message, size_t *data_size);

//char* serializeMessage(struct Message *message);
//struct Message* deserializeMessage(const uint8_t* bytes);

//struct Log* createLog(uint8_t segment_id, enum LogLevel level, const char *format, ...);
size_t getMessageLength(struct Log *log);

#endif // COMMUNICATION_H