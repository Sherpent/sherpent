//
// Created by gabriel on 2/19/25.
//

#ifndef BLE_H
#define BLE_H

#include <stdint.h>
#include <communication.h>
#include <stdbool.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

typedef void (*conn_callback_t)(uint16_t conn_id);
typedef void (*message_callback_t)(uint16_t sender_conn_id, struct Message *message);
typedef void (*event_callback_t)();

typedef struct {
    int conn_id;
    struct Message *message;
} message_queue_item_t;

enum conn_callback_type {
    CONNECTION,
    DISCONNECTION,
};

void init_ble();

bool send_message(uint16_t conn_id, struct Message *message);

bool set_advertising(bool is_advertising);

void register_conn_callback(enum conn_callback_type type, conn_callback_t callback);
void register_msg_callback(message_callback_t callback);
void register_setup_complete_callback(event_callback_t callback);

#endif //BLE_H
