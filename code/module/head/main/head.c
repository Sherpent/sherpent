#include <stdio.h>
#include <ble.h>
#include <head.h>
#include <esp_log.h>
#include <led.h>
#include <safety.h>
#include <servo.h>
#include <power.h>
#include <string.h>

#include "esp_intr_alloc.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"

static bool modules_registered[MAX_MODULE_NUM] = {false};
static uint16_t modules_conn_id[MAX_MODULE_NUM] = {0};

static bool master_connected = false;
static uint16_t master_conn_id = 0;

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

void monitor_battery_task(void *parameters) {
    TaskHandle_t breath_handle = NULL;
    TaskHandle_t flash_handle = NULL;

    for (;;) {
        ESP_LOGI("BATTERY VOLTAGE", "%.2lf V", get_battery_voltage());
        /*
        if (is_battery_charging()) {
            if (breath_handle == NULL) {
                breath_handle = breath(255, 0, 0, 0, 10, 5000);
            }
        } else {
            if (breath_handle != NULL) {
                stop_effect(breath_handle);
                breath_handle = NULL;
            }
        }
        */
        if (get_battery_percentage() < 0.1) { // Under 10%
            if (flash_handle == NULL) {
                flash_handle = flash(255, 0, 0, 1000, 0.1);
            }
        } else {
            if (flash_handle != NULL) {
                stop_effect(flash_handle);
                flash_handle = NULL;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}


void app_main(void)
{
    safety_init();
    power_init();
    set_powered(true);

    register_conn_callback(CONNECTION, on_connect);
    register_conn_callback(DISCONNECTION, on_disconnect);
    register_msg_callback(_message_callback);
    register_setup_complete_callback(ble_main);

    led_init();
    servo_init();
    init_ble();

    button_setup();
    xTaskCreate(monitor_battery_task, "MonitorCharge", 4096, NULL, 2, NULL);

    set_pixel_rgb(0, 0, 50, 0);
    set_servo_angle(YAW, (float) 0);
    set_servo_angle(PITCH, (float) 0);
while(true) //Added RG
{
   set_servo_angle(YAW, (float) 0);
   set_servo_angle(PITCH, (float) 0);
   vTaskDelay(pdMS_TO_TICKS(500)); // Sleep for 1000 ms
   set_servo_angle(YAW, (float) 90);
   vTaskDelay(pdMS_TO_TICKS(500)); // Sleep for 1000 ms
   set_servo_angle(PITCH, (float) 90);
   vTaskDelay(pdMS_TO_TICKS(500)); // Sleep for 1000 ms
   set_servo_angle(YAW, (float) -90);
   vTaskDelay(pdMS_TO_TICKS(500)); // Sleep for 1000 ms
   set_servo_angle(PITCH, (float) -90);
   vTaskDelay(pdMS_TO_TICKS(500)); // Sleep for 1000 ms
}
}

void ble_main(void) {
    set_advertising(true);
}

void on_connect(uint16_t conn_id) {
    set_advertising(true);
    burst(0, 100, 0, 500);
}

void on_disconnect(uint16_t conn_id) {
    if (is_master_conn_id(conn_id)) unregister_master();

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
        ESP_LOGW("MODULE_MANAGER", "module #%d is not registered, but trying to find it's connection id was attempted", segment_id);
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

    if (message->msg_id == REGISTER_MASTER) {
        struct Register *register_message = (struct Register*) message;
        register_module(sender_conn_id, register_message->segment_id);
        burst(0, 140, 255, 500);
        return;
    }

    if (is_master_conn_id(sender_conn_id)) {
        master_message_callback(message);
        return;
    }

    uint8_t segment_id = get_segment_id(sender_conn_id);
    if (segment_id == 0) {
        ESP_LOGW("MODULE_MANAGER", "Message was received from non-registered module, conn_id = %d", sender_conn_id);
    } else message_callback(get_segment_id(sender_conn_id), message);
}

bool is_master_conn_id(uint16_t conn_id) {
    return is_master_connected() && master_conn_id == conn_id;
}

bool is_master_connected() {
    return master_connected;
}

void register_master(uint16_t conn_id) {
    if (!is_master_connected()) {
        ESP_LOGW("MASTER_MANAGER", "Overriding conn_id of master (Master was already registered), %d -> %d", master_conn_id, conn_id);
    }
    master_conn_id = conn_id;
    master_connected = true;
}

void unregister_master() {
    if (!is_master_connected()) {
        ESP_LOGW("MASTER_MANAGER", "Attempted to unregister master while no master is registered, aborting");
        master_conn_id = 0;
        master_connected = false;
    }
}

void register_module(uint16_t conn_id, uint8_t segment_id) {
    if (segment_id == 0) ESP_LOGW("MODULE_MANAGER", "Attempted to register module #0 which can't have a connection id as it represents itself");
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
                 "Attempted to unregister module #0 which doesn't have a connection id as it represents itself");
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

    if (segment_id != 0) {
        send_message(get_conn_id(segment_id), message);
    } else {
        message_callback(segment_id, message);
    }
}

void message_callback(uint8_t segment_id, struct Message *message) {
    burst(255, 140, 0, 500);

    switch (message->msg_id) {
        case LOG: {
            struct Log *log = (struct Log *) message;
            switch (log->log_level) {
                case DEBUG:
                    ESP_LOGD("LOG", "Module #%d - %.*s", segment_id, getMessageLength(log), log->message);
                    break;
                case INFO:
                    ESP_LOGI("LOG", "Module #%d - %.*s", segment_id, (int) getMessageLength(log), log->message);
                    break;
                case WARNING:
                    ESP_LOGW("LOG", "Module #%d - %.*s", segment_id, (int) getMessageLength(log), log->message);
                    break;
                case ERROR:
                    ESP_LOGE("LOG", "Module #%d - %.*s", segment_id, (int) getMessageLength(log), log->message);
                    break;
            }
            break;
        }
        case INFO_BATTERY: {
            struct InfoBattery *info_battery = (struct InfoBattery *) message;
            ESP_LOGI("LOG", "Module #%d [Battery level] - %.2f %%", segment_id, (float) info_battery->level * (100.0f / 255.0f));
            break;
        }
        case SET_YAW: {
            struct SetYaw *set_yaw = (struct SetYaw *) message;
            set_servo_angle(YAW, (float) set_yaw->angle_degrees);
            break;
        }
        case SET_PITCH: {
            struct SetPitch *set_pitch = (struct SetPitch *) message;
            set_servo_angle(PITCH, (float) set_pitch->angle_degrees);
            break;
        }
        case SET_LIGHT: {
            struct SetLight *set_light = (struct SetLight *) message;
            for (int i = 0; i < PIXEL_COUNT; i++) {
                set_pixel_rgb(i, set_light->red, set_light->green, set_light->blue);
            }
            break;
        }
        default:
            break;
    }
    //struct SetLight sent_message = {5, 9, 0, 255, 0};
    //send_message_to_module(1, (struct Message*) &sent_message);
}

void send_message_to_master(struct Message *message) {
    if (is_master_connected()) {
        ESP_LOGW("MODULE_MANAGER", "Attempted to send a message to a non master");
    }

    send_message(get_conn_id(master_conn_id), message);
    burst(140, 0, 255, 500);
}

void master_message_callback(struct Message *message) {
    burst(140, 0, 255, 500);
}
