/**
 ******************************************************************************
 * @file        : tune.cpp
 * @brief       : Tune singleton library implementation
 * @author      : Auto Generated
 * @date        : 2024
 ******************************************************************************
 * @copyright   : Copyright (c) 2024
 * @attention   : SPDX-License-Identifier: MIT
 ******************************************************************************
 */

#include "tune.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cstring>

const char* Tune::kTag = "tune";

Tune::Tune() 
    : buzzer_(nullptr)
    , message_queue_(nullptr)
    , task_handle_(nullptr)
    , playing_mutex_(nullptr)
    , is_initialized_(false)
    , is_playing_(false) {
}

Tune::~Tune() {
    if (task_handle_) {
        vTaskDelete(task_handle_);
    }
    if (message_queue_) {
        vQueueDelete(message_queue_);
    }
    if (playing_mutex_) {
        vSemaphoreDelete(playing_mutex_);
    }
    if (buzzer_) {
        delete buzzer_;
    }
}

Tune& Tune::getInstance() {
    static Tune instance;
    return instance;
}

bool Tune::initialize(gpio_num_t gpio_num, 
                     ledc_clk_cfg_t clk_cfg,
                     ledc_mode_t speed_mode,
                     ledc_timer_bit_t timer_bit,
                     ledc_timer_t timer_num,
                     ledc_channel_t channel) {
    if (is_initialized_) {
        ESP_LOGW(kTag, "Tune already initialized");
        return true;
    }
    
    // 创建 Buzzer 实例
    ledc_timer_config_t timer_config;
    ledc_channel_config_t channel_config;
    
    Buzzer::MakeConfig(&timer_config, &channel_config, gpio_num, clk_cfg, 
                       speed_mode, timer_bit, timer_num, channel);
    
    buzzer_ = new Buzzer(&timer_config, &channel_config);
    if (!buzzer_) {
        ESP_LOGE(kTag, "Failed to create buzzer");
        return false;
    }
    
    // 创建消息队列
    message_queue_ = xQueueCreate(kQueueSize, sizeof(TuneMessage));
    if (!message_queue_) {
        ESP_LOGE(kTag, "Failed to create message queue");
        delete buzzer_;
        buzzer_ = nullptr;
        return false;
    }
    
    // 创建播放状态互斥锁
    playing_mutex_ = xSemaphoreCreateMutex();
    if (!playing_mutex_) {
        ESP_LOGE(kTag, "Failed to create playing mutex");
        vQueueDelete(message_queue_);
        delete buzzer_;
        buzzer_ = nullptr;
        return false;
    }
    
    // 创建任务
    BaseType_t res = xTaskCreate(taskForwarder, "tune_task", kStackSize, 
                                this, uxTaskPriorityGet(nullptr) + 1, &task_handle_);
    if (res != pdPASS) {
        ESP_LOGE(kTag, "Failed to create tune task");
        vQueueDelete(message_queue_);
        vSemaphoreDelete(playing_mutex_);
        delete buzzer_;
        buzzer_ = nullptr;
        return false;
    }
    
    is_initialized_ = true;
    ESP_LOGI(kTag, "Tune initialized successfully on GPIO %d", (int)gpio_num);
    return true;
}

void Tune::taskForwarder(void* pvParameters) {
    static_cast<Tune*>(pvParameters)->tuneTask();
}

void Tune::tuneTask() {
    TuneMessage message(MSG_STOP);
    
    while (true) {
        BaseType_t res = xQueueReceive(message_queue_, &message, portMAX_DELAY);
        if (res != pdPASS) {
            continue;
        }
        
        switch (message.type) {
            case MSG_MELODY:
                if (message.melody && !message.melody->empty()) {
                    ESP_LOGD(kTag, "Playing melody with %d notes", message.melody->size());
                    
                    xSemaphoreTake(playing_mutex_, portMAX_DELAY);
                    is_playing_ = true;
                    xSemaphoreGive(playing_mutex_);
                    
                    // 播放旋律
                    bool interrupted = false;
                    for (const auto& note : *message.melody) {
                        // 检查是否有新消息（中断旋律）
                        TuneMessage temp_msg(MSG_STOP);
                        if (xQueueReceive(message_queue_, &temp_msg, 0) == pdPASS) {
                            if (temp_msg.type == MSG_MELODY) {
                                ESP_LOGD(kTag, "Melody interrupted by new melody");
                                message = temp_msg;
                                interrupted = true;
                                break;
                            } else if (temp_msg.type == MSG_GEIGER_CLICK) {
                                playNote(Note(kGeigerFreq, kGeigerDuration));
                            } else if (temp_msg.type == MSG_STOP) {
                                interrupted = true;
                                break;
                            }
                        }
                        
                        playNote(note);
                    }
                    
                    if (interrupted && message.type == MSG_MELODY) {
                        // 如果被新旋律中断，继续播放新旋律
                        continue;
                    }
                    
                    xSemaphoreTake(playing_mutex_, portMAX_DELAY);
                    is_playing_ = false;
                    xSemaphoreGive(playing_mutex_);
                    ESP_LOGD(kTag, "Melody finished");
                }
                break;
                
            case MSG_GEIGER_CLICK:
                ESP_LOGD(kTag, "Geiger click");
                playNote(Note(kGeigerFreq, kGeigerDuration));
                break;
                
            case MSG_STOP:
            stopBuzzer();  // 立即停止当前音符
            xSemaphoreTake(playing_mutex_, portMAX_DELAY);
            is_playing_ = false;
            xSemaphoreGive(playing_mutex_);
            break;
        }
    }
}

void Tune::playNote(const Note& note) {
    if (note.frequency == 0) {
        // 休止符 - 只是等待，不发送任何频率到 buzzer
        ESP_LOGD(kTag, "Playing rest for %d ms", note.duration_ms);
        vTaskDelay(pdMS_TO_TICKS(note.duration_ms));
    } else {
        // 播放音符
        ESP_LOGD(kTag, "Playing note %d Hz for %d ms", note.frequency, note.duration_ms);
        buzzer_->Beep(note.frequency, note.duration_ms);
        
        // 等待音符播放完成
        vTaskDelay(pdMS_TO_TICKS(note.duration_ms));
        
        // 音符间的短暂停顿（让音符更清晰）
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

bool Tune::playMelody(const Melody& melody) {
    if (!is_initialized_) {
        ESP_LOGE(kTag, "Tune not initialized");
        return false;
    }
    
    if (melody.empty()) {
        ESP_LOGW(kTag, "Empty melody");
        return false;
    }
    
    // 复制旋律到成员变量中（避免引用失效）
    current_melody_ = melody;
    
    TuneMessage message(MSG_MELODY, &current_melody_);
    BaseType_t res = xQueueSend(message_queue_, &message, 0);
    if (res != pdPASS) {
        ESP_LOGW(kTag, "Failed to send melody message, queue full");
        return false;
    }
    
    return true;
}

bool Tune::geigerClick() {
    if (!is_initialized_) {
        ESP_LOGE(kTag, "Tune not initialized");
        return false;
    }
    
    TuneMessage message(MSG_GEIGER_CLICK);
    BaseType_t res = xQueueSend(message_queue_, &message, 0);
    if (res != pdPASS) {
        return false;
    }
    
    return true;
}

void Tune::stop() {
    if (!is_initialized_) {
        return;
    }
    
    TuneMessage message(MSG_STOP);
    xQueueSend(message_queue_, &message, 0);
}

bool Tune::isPlaying() const {
    if (!is_initialized_ || !playing_mutex_) {
        return false;
    }
    
    bool playing = false;
    if (xSemaphoreTake(playing_mutex_, pdMS_TO_TICKS(10)) == pdTRUE) {
        playing = is_playing_;
        xSemaphoreGive(playing_mutex_);
    }
    
    return playing;
}

// C 接口实现
extern "C" {
    bool tune_initialize(gpio_num_t gpio_num) {
        return Tune::getInstance().initialize(gpio_num);
    }
    
    bool tune_play_melody(const tune_note_t* notes, size_t note_count) {
        if (!notes || note_count == 0) {
            return false;
        }
        
        Tune::Melody melody;
        melody.reserve(note_count);
        
        for (size_t i = 0; i < note_count; i++) {
            melody.emplace_back(notes[i].frequency, notes[i].duration_ms);
        }
        
        return Tune::getInstance().playMelody(melody);
    }
    
    bool tune_geiger_click(void) {
        return Tune::getInstance().geigerClick();
    }
    
    void tune_stop(void) {
        Tune::getInstance().stop();
    }
    
    bool tune_is_playing(void) {
        return Tune::getInstance().isPlaying();
    }
}

void Tune::stopBuzzer() {
    if (buzzer_) {
        // a reaaaaally short note to act as stop.
        buzzer_->Beep(1, 1);
    }
}