//
// Created by Mathieu Durand on 2025-02-11.
//

#ifndef SEGMENT_BLE_H
#define SEGMENT_BLE_H
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Attributes State Machine */
enum
{
    IDX_SVC,

    IDX_CHAR_CHANNEL,
    IDX_CHAR_VAL_CHANNEL,

    IDX_CHAR_ID,
    IDX_CHAR_VAL_ID,

    HRS_IDX_NB,
};

/* ============================== TYPE DEFINITIONS =============================== */
typedef void (*message_handler_t)(struct Message*, enum Target from);
typedef void (*scan_handler_t)(struct ble_scan_result_evt_param result);
typedef void (*message_provider_t)(struct Message*, enum Target to);
typedef void (*connect_event_t)(esp_bd_addr_t address, esp_ble_addr_type_t address_type, uint16_t connection_id);
typedef void (*disconnect_event_t)(esp_bd_addr_t address);
typedef void (*closure_t)();

/**
 * Checks if BLE device is ready to advertise
 * @return is BLE device ready to advertise?
 */
bool is_advertisement_ready();

/**
 * Start advertising BLE device
 * @return True if successful and False is unsuccessful
 */
bool start_advertising();

/**
 * Stop advertising BLE device
 * @return True if successful and False is unsuccessful
 */
bool stop_advertising();

/**
 * Start scanning for other BLE device for a given period of time
 * @param duration scan duration in seconds
 * @return rue if successful and False is unsuccessful
 */
bool start_scanning(uint32_t duration);

/**
 * Stop scanning for other BLE device
 * @return True if successful and False is unsuccessful
 */
bool stop_scanning();

/**
 * Connect a Public address type BLE server device
 * @param address address of the device to connect to
 * @param address_type type of the address either BLE_ADDR_TYPE_RANDOM or BLE_ADDR_TYPE_PUBLIC (comes with a fee)
 * @return True if successful and False is unsuccessful
 */
bool connect_device(esp_bd_addr_t address, esp_ble_addr_type_t address_type);

/**
 * Disconnect a from a device
 * @param connection_id The server's connection id assigned by the client
 * @return True if successful and False is unsuccessful
 */
bool disconnect_device(uint16_t connection_id);

/**
 * Disconnect a remote device
 * @param connection_id The connection id assigned by the BLE server
 * @return True if successful and False is unsuccessful
 */
bool disconnect_remote_device(uint16_t connection_id);

#endif //SEGMENT_BLE_H
