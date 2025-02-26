//
// Created by Mathieu Durand on 2025-02-11.
//
#include <stdbool.h>

#ifndef HEAD_H
#define HEAD_H

#define MAX_MODULE_NUM 9

/**
 * Verify if a modules with a specific ID is registered
 * @param segment_id The segment ID
 * @return True if the module is registered, false if not
 */
bool is_segment_id_registered(uint8_t segment_id);

/**
 * Get the segment ID corresponding to a specific connection ID
 * @param conn_id The connection ID for which we are trying to find the segment ID
 * @return > 1 if the found and 0 if not found
 */
uint8_t get_segment_id(uint16_t conn_id);

/**
 * Get the connection ID corresponding to a specific segment
 * @param segement The segment for which we are trying to find the connection ID
 * @return The associated segment ID (Note: will return 0 if it can't find it, is_segment_id_registered(segment_id) should always be called before this function)
 */
uint16_t get_conn_id(uint8_t segment_id);

void register_module(uint16_t conn_id, uint8_t segment_id);
void unregister_module(uint8_t segment_id);
void _message_callback(uint16_t sender_conn_id, struct Message *message);

void ble_main(void);
void on_connect(uint16_t conn_id);
void on_disconnect(uint16_t conn_id);

void send_message_to_module(uint8_t segment_id, struct Message *message);
void message_callback(uint8_t segment_id, struct Message *message);

#endif // HEAD_H