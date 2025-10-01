/**
 ******************************************************************************
 * @file        : main.cpp
 * @brief       : Buzzer example
 * @author      : Jacques Supcik <jacques@supcik.net>
 * @date        : 8 July 2024
 ******************************************************************************
 * @copyright   : Copyright (c) 2024 Jacques Supcik
 * @attention   : SPDX-License-Identifier: MIT
 ******************************************************************************
 * @details
 *
 ******************************************************************************
 */

#include "buzzer.hpp"
#include "esp_log.h"

const char* kTag = "buzzer (example)";

extern "C" {
void app_main(void);
}

void PrintStat() {
    UBaseType_t nOfTasks = uxTaskGetNumberOfTasks();
    TaskStatus_t* data = new TaskStatus_t[nOfTasks];
    UBaseType_t res = uxTaskGetSystemState(data, nOfTasks, nullptr);
    if (res == pdFALSE) {
        ESP_LOGE(kTag, "Failed to get task status");
        delete[] data;
        return;
    }
    ESP_LOGI(kTag, "Number of tasks: %d", nOfTasks);
    for (UBaseType_t i = 0; i < nOfTasks; i++) {
        ESP_LOGI(
            kTag, "Task %d: %s, SHW=%" PRIu32, i, data[i].pcTaskName, data[i].usStackHighWaterMark);
    }
    delete[] data;
}

void app_main(void) {
    ESP_LOGI(kTag, "Starting buzzer test");
    ledc_timer_config_t timer_config;
    ledc_channel_config_t channel_config;
    Buzzer::MakeConfig(&timer_config, &channel_config, GPIO_NUM_11);
    Buzzer buzzer(&timer_config, &channel_config);
    while (true) {
        ESP_LOGI(kTag, "Beep");
        for (int i = 1000; i < 9000; i += 500) {
            buzzer.Beep(i, 1000);
            vTaskDelay(pdMS_TO_TICKS(500));
        }
        PrintStat();
    }
}
