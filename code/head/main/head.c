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

// send_message_forward
// send_message_backward

void app_main(void) {
    char name[ESP_BLE_ADV_NAME_LEN_MAX] = "SHERPANT_MODULE";
    ble_main(name, parse_received_message, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
}