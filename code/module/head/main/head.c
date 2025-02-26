#include <stdio.h>
#include <ble.h>
#include <head.h>
#include <esp_log.h>
#include <led.h>
#include <power.h>
#include <string.h>

#include "esp_intr_alloc.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"

static bool modules_registered[MAX_MODULE_NUM] = {false};
static uint16_t modules_conn_id[MAX_MODULE_NUM] = {0};

void rgb_task(void *pvParameters);

// Interrupt handler
static void IRAM_ATTR button_isr_handler(void* arg) {
    // Code to execute when button is pressed
    set_powered(false);
}

void button_setup() {
    gpio_config_t io_conf = {
            .pin_bit_mask = (1ULL << BUTTON_PIN),  // Set the GPIO pin number
            .mode = GPIO_MODE_INPUT,                // Set as input
            .pull_up_en = GPIO_PULLUP_ENABLE,       // Enable internal pull-up resistor
            .intr_type = GPIO_INTR_POSEDGE,         // Trigger interrupt on rising edge
    };
    gpio_config(&io_conf);

    // Install the GPIO interrupt service
    gpio_install_isr_service(ESP_INTR_FLAG_LEVEL1);

    // Attach the interrupt handler to the GPIO pin
    gpio_isr_handler_add(BUTTON_PIN, button_isr_handler, NULL);
}


void app_main(void)
{
    power_init();
    set_powered(true);

    register_conn_callback(CONNECTION, on_connect);
    register_conn_callback(DISCONNECTION, on_disconnect);
    register_msg_callback(_message_callback);
    register_setup_complete_callback(ble_main);

    led_init();
    init_ble();

    button_setup();
}

void ble_main(void) {
    set_advertising(true);
}

void on_connect(uint16_t conn_id) {
    set_advertising(true);
    burst(0, 255, 0, 500);
}

void on_disconnect(uint16_t conn_id) {
    uint8_t segment_id = get_segment_id(conn_id);
    if (segment_id != 0) unregister_module(get_segment_id(conn_id));

    burst(255, 0, 0, 500);
}

// ================================ MODULE MANAGEMENT ================================

bool is_segment_id_registered(uint8_t segment_id) {
    if (segment_id == 0) return true;
    return modules_registered[segment_id - 1];
}

uint8_t get_segment_id(uint16_t conn_id) {
    for (uint8_t i = 0; i < MAX_MODULE_NUM; i++) {
        if (modules_registered[i] && conn_id == modules_conn_id[i]) {
            return i + 1;
        }
    }

    return 0;
}

uint16_t get_conn_id(uint8_t segment_id) {
    if (!is_segment_id_registered(segment_id)) {
        ESP_LOGW("MODULE_MANAGER", "module #%d is not registered, but trying to find it's connection id was attempted");
        return 0;
    }

    if (segment_id == 0) {
        ESP_LOGW("MODULE_MANAGER", "module #0 doesn't have a connection id as it represents itself, but trying to find it's connection id was attempted");
        return 0;
    }

    return modules_conn_id[segment_id - 1];
}

void _message_callback(uint16_t sender_conn_id, struct Message *message) {
    if (message->msg_id == REGISTER) {
        struct Register *register_message = (struct Register*) message;
        register_module(sender_conn_id, register_message->segment_id);
        burst(0, 140, 255, 500);
        return;
    }


    uint8_t segment_id = get_segment_id(sender_conn_id);
    if (segment_id == 0) {
        ESP_LOGW("MODULE_MANAGER", "Message was received from non-registered module, conn_id = %d", sender_conn_id);
    } else message_callback(get_segment_id(sender_conn_id), message);
}

void register_module(uint16_t conn_id, uint8_t segment_id) {
    if (segment_id == 0) ESP_LOGW("MODULE_MANAGER", "Attempted to register module #0 which can't have a connection id as it represents itself", segment_id, get_conn_id(segment_id), conn_id);
    else {
        if (is_segment_id_registered(segment_id))
            ESP_LOGW("MODULE_MANAGER", "Overriding conn_id of module #%d (Module was already registered), %d -> %d",
                     segment_id, get_conn_id(segment_id), conn_id);

        modules_registered[segment_id - 1] = true;
        modules_conn_id[segment_id - 1] = conn_id;
        ESP_LOG_BUFFER_HEX("MODULE_MANAGER", modules_registered, sizeof(modules_registered));
        ESP_LOG_BUFFER_HEX("MODULE_MANAGER", modules_conn_id, sizeof(modules_conn_id));
    }
}

void unregister_module(uint8_t segment_id) {
    if (segment_id == 0) {
        ESP_LOGW("MODULE_MANAGER",
                 "Attempted to unregister module #0 which doesn't have a connection id as it represents itself",
                 segment_id);
    } else {
        if (!is_segment_id_registered(segment_id))
            ESP_LOGW("MODULE_MANAGER", "Attempted to unregister non registered module #%d",  segment_id);

        modules_registered[segment_id - 1] = false;
        modules_conn_id[segment_id - 1] = 0;
    }
}

// ================================= MESSAGE HANDLING ================================

void send_message_to_module(uint8_t segment_id, struct Message *message) {
    if (!is_segment_id_registered(segment_id)) {
        ESP_LOGW("MODULE_MANAGER", "Attempted to send a message to a non registered module #%d", segment_id);
    }

    send_message(get_conn_id(segment_id), message);
}

void message_callback(uint8_t segment_id, struct Message *message) {
    burst(255, 140, 0, 500);
    struct SetLight sent_message = {5, 9, 0, 255, 0};
    send_message_to_module(1, (struct Message*) &sent_message);
}