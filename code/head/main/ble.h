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

#endif //SEGMENT_BLE_H
