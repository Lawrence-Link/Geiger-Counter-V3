#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "u8g2_port.h"
#include "U8g2lib.h"

#include <inttypes.h>
#include <string.h>
#include <encoder.h>

#include "PixelUI.h"
#include "ui/AppView/AppView.h"
#include "app_registry.h"
#include "pin_definitions.h"

#define EV_QUEUE_LEN 5
#define INPUT_QUEUE_LEN 10   // queue length for input events

// Global variables
U8G2 u8g2;
PixelUI ui(u8g2);
static const char *TAG = "main";
static QueueHandle_t encoder_event_queue;
static QueueHandle_t input_event_queue;   // input event queue
static rotary_encoder_t re;

void delay_ms(uint32_t ms) {
    vTaskDelay(pdMS_TO_TICKS(ms));
}

// UI heartbeat task (only heartbeat, no input here)
void ui_heartbeat_task(void* p) {
    const TickType_t xFrequency = pdMS_TO_TICKS(10);
    TickType_t xLastWakeTime = xTaskGetTickCount();

    while (1) {
        ui.Heartbeat(10);
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

// Encoder handling task (send InputEvent to queue)
void encoder_task(void* p) {
    encoder_event_queue = xQueueCreate(EV_QUEUE_LEN, sizeof(rotary_encoder_event_t));
    ESP_ERROR_CHECK(rotary_encoder_init(encoder_event_queue));

    memset(&re, 0, sizeof(rotary_encoder_t));
    re.pin_a = ENC_A_GPIO;
    re.pin_b = ENC_B_GPIO;
    re.pin_btn = ENC_BTN_GPIO;
    ESP_ERROR_CHECK(rotary_encoder_add(&re));

    ESP_LOGI(TAG, "Encoder initialized on pins A:%d, B:%d, BTN:%d", ENC_A_GPIO, ENC_B_GPIO, ENC_BTN_GPIO);

    rotary_encoder_event_t e;
    InputEvent inputEv;

    while (1) {
        if (xQueueReceive(encoder_event_queue, &e, portMAX_DELAY) == pdTRUE) {
            switch (e.type) {
                case RE_ET_BTN_CLICKED:
                    inputEv = InputEvent::SELECT;
                    xQueueSend(input_event_queue, &inputEv, 0);
                    break;
                case RE_ET_BTN_LONG_PRESSED:
                    inputEv = InputEvent::BACK;
                    xQueueSend(input_event_queue, &inputEv, 0);
                    break;
                case RE_ET_CHANGED:
                    if (e.diff < 0) {
                        inputEv = InputEvent::RIGHT;
                        xQueueSend(input_event_queue, &inputEv, 0);
                    } else if (e.diff > 0) {
                        inputEv = InputEvent::LEFT;
                        xQueueSend(input_event_queue, &inputEv, 0);
                    }
                    break;
                default:
                    break;
            }
        }
    }
}

extern "C" void app_main(void)
{   
    vTaskDelay(pdMS_TO_TICKS(500));

    // Initialize display
    esp_err_t ret = u8g2_init_sh1106(u8g2.getU8g2());
    if (ret != ESP_OK) {
        ESP_LOGI(TAG, "Failed to initialize U8G2: %s\n", esp_err_to_name(ret));
        return;
    }

    // Register applications
    registerApps();

    // Create AppView and push it to ViewManager
    auto appView = std::make_shared<AppView>(ui, *ui.getViewManagerPtr());
    ui.getViewManagerPtr()->push(appView);

    // Initialize UI
    ui.setDelayFunction(delay_ms);
    ui.begin();

    // Create input event queue
    input_event_queue = xQueueCreate(INPUT_QUEUE_LEN, sizeof(InputEvent));

    // Create UI heartbeat task
    xTaskCreate(ui_heartbeat_task, "UI_Heartbeat", 8192, NULL, 3, NULL);

    // Create encoder task
    xTaskCreate(encoder_task, "Encoder_Task", 4096, NULL, 4, NULL);

    // Main rendering loop
    InputEvent ev;
    for (;;) {
        // Process input events
        while (xQueueReceive(input_event_queue, &ev, 0) == pdTRUE) {
            ui.handleInput(ev);
        }

        // Render UI
        ui.renderer();

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
