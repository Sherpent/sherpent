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
#include <esp_gap_ble_api.h>

typedef void (*scan_callback_t)();
typedef void (*conn_callback_t)();
typedef void (*conn_ready_callback_t)();
typedef void (*message_callback_t)(struct Message *message);
typedef void (*event_callback_t)();

typedef struct {
    struct Message *message;
} message_queue_item_t;

enum conn_callback_type {
    CONNECTION,
    DISCONNECTION,
};

enum scan_callback_type {
    START,
    STOP,
};

void init_ble();

bool send_message(struct Message *message);
bool start_scan(uint32_t duration);

bool is_connected();

void register_conn_callback(enum conn_callback_type type, conn_callback_t callback);
void register_scan_callback(enum scan_callback_type type, scan_callback_t callback);
void register_conn_ready_callback(conn_ready_callback_t callback);
void register_msg_callback(message_callback_t callback);
void register_setup_complete_callback(event_callback_t callback);

#endif //BLE_H
