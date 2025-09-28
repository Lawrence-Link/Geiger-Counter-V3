#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include "GPIO.h"
#include "pin_definitions.h"

void GPIO_init() {
    gpio_config_t io_conf = {}; // 使用 {} 初始化结构体的所有成员为 0

    // 2. 配置引脚模式
    io_conf.intr_type = GPIO_INTR_DISABLE;      // 禁用中断
    io_conf.mode = GPIO_MODE_OUTPUT;            // 设置为输出模式
    io_conf.pin_bit_mask = (1ULL << PIN_BUZZER); // 设置要配置的引脚的位掩码
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE; // 禁用下拉电阻
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;   // 禁用上拉电阻

    gpio_config(&io_conf);
}