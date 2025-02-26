#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>

// Message ID Enum
enum MessageID: uint8_t {
    LOG,
    REGISTER,
    INFO_BATTERY,

    SET_YAW,
    SET_PITCH,
    SET_LIGHT,
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


struct InfoBattery {
    uint8_t msg_size;
    enum MessageID msg_id;
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

struct SetLight {
    uint8_t msg_size;
    enum MessageID msg_id;
    uint8_t red;
    uint8_t green;
    uint8_t blue;
};

// Function Declarations
//struct Message* packageMessage(enum MessageID message_id, void* data, size_t data_size);
//void* getMessageData(struct Message *message, size_t *data_size);

//char* serializeMessage(struct Message *message);
//struct Message* deserializeMessage(const uint8_t* bytes);

//struct Log* createLog(uint8_t segment_id, enum LogLevel level, const char *format, ...);
size_t getMessageLength(struct Log *log);

#endif // COMMUNICATION_H