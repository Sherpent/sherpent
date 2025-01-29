#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include "memory.h"
#include "communication.h"

/*
struct Message* packageMessage(enum MessageID message_id, void* data, size_t data_size) {
    // Calculate the total size of the message
    const size_t msg_size = sizeof(struct Message) + data_size;

    // Allocate memory for the struct
    struct Message* message = malloc(msg_size);
    if (message == NULL) {
        perror("Failed to allocate memory for message");
        return NULL;
    }

    // Copy the data into the flexible array
    memcpy(message->data, data, data_size);

    // Initialize the struct fields
    message->msg_size = msg_size;
    message->msg_id = message_id;

    return message;
}

void* getMessageData(struct Message *message, size_t *data_size) {
    *data_size = message->msg_size - sizeof(struct Message);
    uint8_t *data = (uint8_t *) malloc(*data_size);
    if (data == NULL) {
        perror("Failed to allocate memory for message's data");
        return NULL;
    }

    memcpy(data, message->data, *data_size);

    return data;
}

char* serializeMessage(struct Message *message) {
    char *bytes = (char *) malloc(message->msg_size);
    if (bytes == NULL) {
        perror("Failed to allocate memory for serialization buffer");
        return NULL;
    }

    return bytes;
}

struct Message* deserializeMessage(const uint8_t* bytes) {
    const uint8_t msg_size = bytes[0];

    struct Message* message = malloc(msg_size);
    if (message == NULL) {
        perror("Failed to allocate memory for message deserialization");
        return NULL;
    }

    memcpy(message, bytes, msg_size);
    return message;
}

struct Log* createLog(uint8_t segment_id, enum LogLevel level, const char *format, ...) {
    va_list args;
    va_start(args, format);

    // Calculate the required size for the formatted string
    int required_size = vsnprintf(NULL, 0, format, args) + 1; // +1 for null-terminator
    va_end(args);

    // Allocate the buffer dynamically
    char *buffer = (char *) malloc(required_size);
    if (buffer == NULL) {
        // Handle allocation failure
        perror("Failed to allocate memory for log buffer");
        return NULL;
    }

    // Format the string into the dynamically allocated buffer
    va_start(args, format);
    vsnprintf(buffer, required_size, format, args);
    va_end(args);

    // Allocate memory for the struct
    struct Log *log = malloc(sizeof(struct Log) + required_size);

    log->log_level = level;
    log->segment_id = segment_id;
    memcpy(log->message, buffer, required_size);

    free(buffer);

    return log;
}
*/

size_t getMessageLength(struct Log *log) {
    return log->msg_size - sizeof(struct Log);
}