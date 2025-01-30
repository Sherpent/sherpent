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

void app_main(void) {
    unsigned char mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    char name[50];
    sprintf(name, "SHERPANT_MODULE_%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    ble_main(name, parse_received_message);
}