#include <ble.h>
#include "body.h"

#include <led.h>
#include <power.h>
#include <servo.h>
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

void app_main(void) {
    set_powered(true);
    register_setup_complete_callback(setup);
    register_conn_ready_callback(connected);
    init_ble();

    power_init();
    led_init();
    servo_init();

    button_setup();

    xTaskCreate(rgb_task, "RGB Task", 2048, NULL, 1, NULL);
    start_scan(60);
}

void setup() {
    start_scan(60);
}

void connected() {
    struct Register *message = malloc(sizeof(struct Register));
    message->msg_size = (uint8_t) sizeof(struct Register);
    message->msg_id = REGISTER;
    message->segment_id = (uint8_t) 1;
    send_message((struct Message *) message);
    ESP_LOGD("DEBUG", "Sending message");
}

void rgb_task(void *pvParameters) {
    int hue = 0;

    while (1) {
        ESP_LOGI("BATTERY", "%f V", get_battery_voltage());


        for (int i = 0; i < PIXEL_COUNT; i++) {
            int shifted_hue = (hue + (0 * 25)) % 360; // Shift hue for a rainbow effect
            int r, g, b;

            // Simple HSV to RGB conversion
            if (shifted_hue < 120) {
                r = (120 - shifted_hue) * 255 / 120;
                g = shifted_hue * 255 / 120;
                b = 0;
            } else if (shifted_hue < 240) {
                shifted_hue -= 120;
                r = 0;
                g = (120 - shifted_hue) * 255 / 120;
                b = shifted_hue * 255 / 120;
            } else {
                shifted_hue -= 240;
                r = shifted_hue * 255 / 120;
                g = 0;
                b = (120 - shifted_hue) * 255 / 120;
            }

            set_pixel_rgb(i, r, g, b);
        }

        hue = (hue + 5) % 360; // Increment hue for smooth transition
        vTaskDelay(pdMS_TO_TICKS(20)); // Delay for animation speed
    }
}