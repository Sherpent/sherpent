//
// Created by gabriel on 2/19/25.
//

#ifndef BLE_H
#define BLE_H
#include "esp_gap_ble_api.h"

void init_ble();

static void scan_init();
static void scan_result_cb(esp_ble_gap_cb_param_t *scan_result);

#endif //BLE_H
