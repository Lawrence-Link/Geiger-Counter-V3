#include "encoder_task.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "pin_definitions.h"
#include "tune.h"
#include "system_nvs_varibles.h"

static QueueHandle_t encoder_event_queue;
static rotary_encoder_t re;

// Global input queue (shared with main/ui)
QueueHandle_t input_event_queue = nullptr;

Tune::Melody navi_up = {
    {Notes::C5, 40},
    {Notes::G5, 40},
};

Tune::Melody navi_dn = {
    {Notes::G5, 40},
    {Notes::C5, 40},
};

Tune::Melody navi_sel = {
    {Notes::G5, 40},
    {Notes::G5, 40},
};

static void encoder_task(void* p) {
    encoder_event_queue = xQueueCreate(5, sizeof(rotary_encoder_event_t));
    ESP_ERROR_CHECK(rotary_encoder_init(encoder_event_queue));

    memset(&re, 0, sizeof(rotary_encoder_t));
    re.pin_a = PIN_ENCODER_A;
    re.pin_b = PIN_ENCODER_B;
    re.pin_btn = PIN_ENCODER_SW;
    ESP_ERROR_CHECK(rotary_encoder_add(&re));

    rotary_encoder_event_t e;
    InputEvent inputEv;

    auto& tune = Tune::getInstance();
    bool en_navi_tune;

    while (1) {
        if (xQueueReceive(encoder_event_queue, &e, portMAX_DELAY) == pdTRUE) {
             en_navi_tune = SystemConf::getInstance().read_conf_enable_navi_tone();
            switch (e.type) {
                case RE_ET_BTN_CLICKED:
                    inputEv = InputEvent::SELECT;
                    if (en_navi_tune) tune.playMelody(navi_sel);
                    break;
                case RE_ET_BTN_LONG_PRESSED:
                    inputEv = InputEvent::BACK;
                    if (en_navi_tune) tune.playMelody(navi_sel);
                    break;
                case RE_ET_CHANGED:
                    if (e.diff < 0) {
                        inputEv = InputEvent::RIGHT;
                        if (en_navi_tune) tune.playMelody(navi_dn);
                    } else if (e.diff > 0) {
                        inputEv = InputEvent::LEFT;
                        if (en_navi_tune) tune.playMelody(navi_up);
                    } else continue;
                    break;
                default:
                    continue;
            }
            xQueueSend(input_event_queue, &inputEv, 0);
        }
    }
}

void start_encoder_task() {
    // Create input event queue once here
    if (!input_event_queue) {
        input_event_queue = xQueueCreate(10, sizeof(InputEvent));
    }
    xTaskCreate(encoder_task, "Encoder_Task", 4096, NULL, 4, NULL);
}
