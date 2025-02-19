#include <inttypes.h>
#include <esp_mac.h>
#include "esp_log.h"

#include "esp_bt_defs.h"
#include "communication.h"
#include "ble.c"

#include "sdkconfig.h"

void parse_received_message(struct Message *message) {
    switch (message->msg_id) {
        case LOG: {
            struct Log *log = (struct Log*) message;
            ESP_LOGI(TAG, "Received log from segment #%d, [%d] - %.*s", log->segment_id, log->log_level, getMessageLength(log), log->message);
            break;
        }
        default:
            break;
    }
}

void on_scan_result(char *name, uint8_t name_length, esp_bd_addr_t address, esp_ble_addr_type_t address_type) {
    char *target_name = "SHERPANT_MODULE_A";

    ESP_LOGI("MAIN", "Found device: %.*s", name_length, name);
    if (name_length == strlen(target_name) && strcmp(name, target_name) != 0) {
        connect_device(address, address_type);
        ESP_LOGI("MAIN", "Connecting to %.*s", name_length, target_name);
    }
}

void on_scan_stop() {
    ESP_LOGI("MAIN", "Done scanning...");
}

// send_message_forward
// send_message_backward

void app_main(void) {
    char name[ESP_BLE_ADV_NAME_LEN_MAX] = "SHERPANT_MODULE_B";
    ble_main(name, parse_received_message, NULL, NULL, on_scan_stop, on_scan_result, NULL, NULL, NULL, NULL, NULL, NULL);

    vTaskDelay(1000 / portTICK_PERIOD_MS);

    start_advertising();
    start_scanning(20);
}