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

#define HRPS_HT_MEAS_MAX_LEN            (13)

#define HRPS_MANDATORY_MASK             (0x0F)
#define HRPS_BODY_SENSOR_LOC_MASK       (0x30)
#define HRPS_HR_CTNL_PT_MASK            (0xC0)


///Attributes State Machine
enum
{
    HRS_IDX_SVC,

    HRS_IDX_HR_MEAS_CHAR,
    HRS_IDX_HR_MEAS_VAL,
    HRS_IDX_HR_MEAS_NTF_CFG,

    HRS_IDX_BOBY_SENSOR_LOC_CHAR,
    HRS_IDX_BOBY_SENSOR_LOC_VAL,

    HRS_IDX_HR_CTNL_PT_CHAR,
    HRS_IDX_HR_CTNL_PT_VAL,

    HRS_IDX_NB,
};

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
