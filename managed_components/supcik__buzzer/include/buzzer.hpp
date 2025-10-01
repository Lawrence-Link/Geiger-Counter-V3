/**
 ******************************************************************************
 * @file        : buzzer.h
 * @brief       : Buzzer driver
 * @author      : Jacques Supcik <jacques@supcik.net>
 * @date        : 11 March 2024
 ******************************************************************************
 * @copyright   : Copyright (c) 2024 Jacques Supcik
 * @attention   : SPDX-License-Identifier: MIT
 ******************************************************************************
 * @details
 * Buzzer driver for ESP32. The driver uses the LEDC peripheral to generate
 * the sound. The driver is non-blocking and uses a FreeRTOS task to generate
 * the sound at the requested frequency and duration.
 ******************************************************************************
 */

#pragma once

#include "driver/ledc.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

class Buzzer {
   public:
    static void MakeConfig(ledc_timer_config_t* timer_config,
                           ledc_channel_config_t* channel_config,
                           const gpio_num_t gpio_num,
                           const ledc_clk_cfg_t clk_cfg = LEDC_AUTO_CLK,
                           const ledc_mode_t speed_mode = LEDC_LOW_SPEED_MODE,
                           const ledc_timer_bit_t timer_bit = LEDC_TIMER_13_BIT,
                           const ledc_timer_t timer_num = LEDC_TIMER_0,
                           const ledc_channel_t channel = LEDC_CHANNEL_0);

    Buzzer(ledc_timer_config_t* timer_config,
           ledc_channel_config_t* channel_config,
           uint32_t idle_level = 0);
    virtual ~Buzzer();
    BaseType_t Beep(uint32_t frequency, uint32_t duration_ms);

   private:
    struct message_t {
        uint32_t frequency;
        uint32_t duration_ms;
    };

    void Stop();
    void Start(uint32_t frequency);
    void Task();
    static void TaskForwarder(void* param) {
        Buzzer* buzzer = (Buzzer*)param;
        buzzer->Task();
    }

    TaskHandle_t task_handle_;
    QueueHandle_t queue_handle_;
    ledc_mode_t speed_mode_;
    ledc_timer_bit_t timer_bit_;
    ledc_timer_t timer_num_;
    ledc_channel_t channel_;
    uint32_t idle_level_;
};
