#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define PIN_LED_R   GPIO_NUM_6
#define PIN_LED_G   GPIO_NUM_1
#define PIN_LED_B   GPIO_NUM_7

#define BLINK_DURATION_MS   20   // 0.2s

enum class LedColor {
    RED,
    GREEN,
    BLUE,
    YELLOW
};

class LedBlinker {
public:
    static LedBlinker& getInstance() {
        static LedBlinker instance;
        return instance;
    }

    void enqueueBlink(LedColor color) {
        if (queueHandle) {
            xQueueSend(queueHandle, &color, 0);
        }
    }

private:
    LedBlinker() {
        // 配置 GPIO
        gpio_config_t io_conf = {};
        io_conf.mode = GPIO_MODE_OUTPUT;
        io_conf.pin_bit_mask = (1ULL << PIN_LED_R) |
                               (1ULL << PIN_LED_G) |
                               (1ULL << PIN_LED_B);
        io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
        io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
        io_conf.intr_type = GPIO_INTR_DISABLE;
        ESP_ERROR_CHECK(gpio_config(&io_conf));

        // 默认拉高 -> 熄灭
        gpio_set_level(PIN_LED_R, 1);
        gpio_set_level(PIN_LED_G, 1);
        gpio_set_level(PIN_LED_B, 1);

        // 创建队列
        queueHandle = xQueueCreate(10, sizeof(LedColor));
        configASSERT(queueHandle);

        // 创建任务
        xTaskCreate(taskFunc, "LedBlinkTask", 2048, this, 5, nullptr);
    }

    ~LedBlinker() = default;
    LedBlinker(const LedBlinker&) = delete;
    LedBlinker& operator=(const LedBlinker&) = delete;

    static void taskFunc(void* arg) {
    auto* self = static_cast<LedBlinker*>(arg);
    LedColor color;

    while (true) {
        if (xQueueReceive(self->queueHandle, &color, portMAX_DELAY) == pdTRUE) {
            switch (color) {
                case LedColor::RED:
                    gpio_set_level(PIN_LED_R, 0);
                    vTaskDelay(pdMS_TO_TICKS(BLINK_DURATION_MS));
                    gpio_set_level(PIN_LED_R, 1);
                    break;
                case LedColor::GREEN:
                    gpio_set_level(PIN_LED_G, 0);
                    vTaskDelay(pdMS_TO_TICKS(BLINK_DURATION_MS));
                    gpio_set_level(PIN_LED_G, 1);
                    break;
                case LedColor::BLUE:
                    gpio_set_level(PIN_LED_B, 0);
                    vTaskDelay(pdMS_TO_TICKS(BLINK_DURATION_MS));
                    gpio_set_level(PIN_LED_B, 1);
                    break;
                case LedColor::YELLOW: // 红+蓝
                    gpio_set_level(PIN_LED_R, 0);
                    gpio_set_level(PIN_LED_G, 0);
                    vTaskDelay(pdMS_TO_TICKS(BLINK_DURATION_MS));
                    gpio_set_level(PIN_LED_R, 1);
                    gpio_set_level(PIN_LED_G, 1);
                    break;
                default:
                    ESP_LOGW("LED", "Invalid LED color %d!", (int)color);
                    break;
            }
        }
    }
}

    QueueHandle_t queueHandle = nullptr;
};