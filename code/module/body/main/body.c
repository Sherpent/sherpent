#include <ble.h>
#include "body.h"

#include <led.h>
#include <safety.h>
#include <power.h>
#include <servo.h>
#include <storage.h>
#include "esp_log.h"
#include "esp_intr_alloc.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"

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
    TaskHandle_t flash_handle = NULL;

    struct InfoBattery battery_info; // Allocate once
    battery_info.msg_size = sizeof(struct InfoBattery);
    battery_info.msg_id = INFO_BATTERY;

    for (;;) {
        ESP_LOGI("BATTERY VOLTAGE", "%.2lf V", get_battery_voltage());
        float battery_percentage = get_battery_percentage();
        if (battery_percentage < 0.1) { // Under 10%
            if (flash_handle == NULL) {
                flash_handle = flash(255, 0, 0, 1000, 0.1);
            }
        } else {
            if (flash_handle != NULL) {
                stop_effect(flash_handle);
                flash_handle = NULL;
            }
        }

        battery_info.level = battery_percentage * 255;
        send_message((struct Message *) &battery_info);

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}


void app_main(void) {
    safety_init();
    power_init();
    //set_powered(true);

    register_setup_complete_callback(ble_main);
    register_scan_callback(STOP, on_scan_stopped);
    register_conn_ready_callback(on_connected);
    register_conn_callback(DISCONNECTION, on_disconnected);
    init_ble();
    register_msg_callback(message_callback);

    led_init();
    servo_init();

    button_setup();
    xTaskCreate(monitor_battery_task, "MonitorCharge", 4096, NULL, 2, NULL);
    set_pixel_rgb(0, 0, 50, 0);

    uint8_t segment_id;
    if (get_segment_id(&segment_id) == ESP_OK) {
        ESP_LOGI("MODULE INFO", "Current segment_id: %d", segment_id);
    }

    start_scan(60);
}

void ble_main() {
    //start_scan(60);
}

void on_connected() {
    struct Register *message = malloc(sizeof(struct Register));
    message->msg_size = (uint8_t) sizeof(struct Register);
    message->msg_id = REGISTER;
    get_segment_id(&(message->segment_id));
    send_message((struct Message *) message);
    burst(0, 255, 0, 500);
}

void on_disconnected() {
    start_scan(5);
    burst(255, 0, 0, 500);
}

void on_scan_stopped() {
    if (!is_connected()) {
        start_scan(true);
    }
}

void message_callback(struct Message *message) {
    //burst(255, 140, 0, 500);

    switch (message->msg_id) {
        case LOG: {
            struct Log *log = (struct Log *) message;
            switch (log->log_level) {
                case DEBUG:
                    ESP_LOGD("LOG", "%.*s", getMessageLength(log), log->message);
                    break;
                case INFO:
                    ESP_LOGI("LOG", "%.*s", (int) getMessageLength(log), log->message);
                    break;
                case WARNING:
                    ESP_LOGW("LOG", "%.*s", (int) getMessageLength(log), log->message);
                    break;
                case ERROR:
                    ESP_LOGE("LOG", "%.*s", (int) getMessageLength(log), log->message);
                    break;
            }
            break;
        }
        case INFO_BATTERY: {
            struct InfoBattery *info_battery = (struct InfoBattery *) message;
            ESP_LOGI("LOG", "[Battery level] - %.2f %%", (float) info_battery->level * (100.0f / 255.0f));
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