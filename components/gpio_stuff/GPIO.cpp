#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include "GPIO.h"
#include "pin_definitions.h"
#include "common.h"
#include "tune.h"
#include "esp_log.h"

extern QueueHandle_t ui_event_queue;

bool showing_charging_anim = false;
static UI_EVENT event;

static void IRAM_ATTR usb_status_isr_handler(void* arg) {
    if (!showing_charging_anim) {
        showing_charging_anim = true;
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        event = USB_POWER_LVL;
        xQueueSendFromISR(ui_event_queue, &event, &xHigherPriorityTaskWoken);
        if (xHigherPriorityTaskWoken) {
            portYIELD_FROM_ISR();
        }
    }
}

void GPIO_init() {
    gpio_config_t io_conf = {};

    // pin config: output, no intr
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;            
    io_conf.pin_bit_mask = (1ULL << PIN_BUZZER) | (1ULL << PIN_OLED_RST); 
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE; 
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;   

    gpio_config(&io_conf);

    io_conf.intr_type = GPIO_INTR_POSEDGE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = 1ULL << PIN_USB_STATUS;
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);

    gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
    gpio_isr_handler_add(PIN_USB_STATUS, usb_status_isr_handler, (void*) PIN_USB_STATUS);

    Tune& tune = Tune::getInstance();
    if (!tune.initialize(PIN_BUZZER)) {
        ESP_LOGE("kTag", "Failed to initialize tune library");
        return;
    }
}