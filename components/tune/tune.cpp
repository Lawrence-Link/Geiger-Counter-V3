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
    , is_playing_(false)
    , current_melody_state_(nullptr) {
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
                                this, 2, &task_handle_);
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
            case MSG_MELODY: {
                // 普通旋律播放，不保存当前状态
                clearCurrentMelody();
                
                if (message.melody && !message.melody->empty()) {
                    ESP_LOGD(kTag, "Playing melody with %d notes", message.melody->size());
                    
                    xSemaphoreTake(playing_mutex_, portMAX_DELAY);
                    is_playing_ = true;
                    xSemaphoreGive(playing_mutex_);
                    
                    // 创建临时状态用于播放
                    MelodyState temp_state(*message.melody);
                    current_melody_state_ = &temp_state;
                    
                    // 播放旋律
                    for (size_t i = 0; i < temp_state.melody.size(); i++) {
                        // 检查是否有新消息（中断旋律）
                        TuneMessage temp_msg(MSG_STOP);
                        if (xQueueReceive(message_queue_, &temp_msg, 0) == pdPASS) {
                            if (temp_msg.type == MSG_MELODY || temp_msg.type == MSG_MELODY_INTERRUPTIBLE) {
                                ESP_LOGD(kTag, "Melody interrupted by new melody");
                                message = temp_msg;
                                break;
                            } else if (temp_msg.type == MSG_GEIGER_CLICK) {
                                playNote(Note(kGeigerFreq, kGeigerDuration));
                            } else if (temp_msg.type == MSG_STOP) {
                                break;
                            }
                        }
                        
                        playNote(temp_state.melody[i]);
                    }
                    
                    if ((message.type == MSG_MELODY || message.type == MSG_MELODY_INTERRUPTIBLE) && 
                        message.melody && !message.melody->empty()) {
                        // 如果被新旋律中断，继续播放新旋律
                        continue;
                    }
                    
                    current_melody_state_ = nullptr;
                    xSemaphoreTake(playing_mutex_, portMAX_DELAY);
                    is_playing_ = false;
                    xSemaphoreGive(playing_mutex_);
                    ESP_LOGD(kTag, "Melody finished");
                }
                break;
            }
            
            case MSG_MELODY_INTERRUPTIBLE: {
                // 可中断的临时旋律，需要保存当前状态
                pauseCurrentMelody();
                
                if (message.melody && !message.melody->empty()) {
                    ESP_LOGD(kTag, "Playing interruptible melody with %d notes", message.melody->size());
                    
                    xSemaphoreTake(playing_mutex_, portMAX_DELAY);
                    is_playing_ = true;
                    xSemaphoreGive(playing_mutex_);
                    
                    // 播放临时旋律
                    for (const auto& note : *message.melody) {
                        // 检查是否有新消息（中断旋律）
                        TuneMessage temp_msg(MSG_STOP);
                        if (xQueueReceive(message_queue_, &temp_msg, 0) == pdPASS) {
                            if (temp_msg.type == MSG_MELODY || temp_msg.type == MSG_MELODY_INTERRUPTIBLE) {
                                ESP_LOGD(kTag, "Interruptible melody interrupted by new melody");
                                message = temp_msg;
                                break;
                            } else if (temp_msg.type == MSG_GEIGER_CLICK) {
                                playNote(Note(kGeigerFreq, kGeigerDuration));
                            } else if (temp_msg.type == MSG_STOP) {
                                break;
                            }
                        }
                        
                        playNote(note);
                    }
                    
                    if ((message.type == MSG_MELODY || message.type == MSG_MELODY_INTERRUPTIBLE) && 
                        message.melody && !message.melody->empty()) {
                        // 如果被新旋律中断，继续播放新旋律
                        continue;
                    }
                    
                    // 临时旋律播放完成，恢复之前的旋律
                    ESP_LOGD(kTag, "Interruptible melody finished, resuming previous melody");
                    resumeMelody();
                }
                break;
            }
            
            case MSG_GEIGER_CLICK:
                ESP_LOGD(kTag, "Geiger click");
                playNote(Note(kGeigerFreq, kGeigerDuration));
                break;
                
            case MSG_STOP:
                stopBuzzer();  // 立即停止当前音符
                clearCurrentMelody();
                xSemaphoreTake(playing_mutex_, portMAX_DELAY);
                is_playing_ = false;
                xSemaphoreGive(playing_mutex_);
                break;
                
            case MSG_RESUME:
                // 内部消息，由resumeMelody()发送
                if (!melody_stack_.empty()) {
                    resumeMelody();
                }
                break;
        }
    }
}

void Tune::playNote(const Note& note) {
    if (note.frequency == 0) {
        vTaskDelay(pdMS_TO_TICKS(note.duration_ms));
    } else {
        // playNote
        buzzer_->Beep(note.frequency, note.duration_ms);
        
        // wait for note stop
        vTaskDelay(pdMS_TO_TICKS(note.duration_ms));
        
        // slightly add some delay to make notes more distinguishable.
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
    
    // 复制旋律到临时变量中
    temp_melody_ = melody;
    
    TuneMessage message(MSG_MELODY, &temp_melody_);
    BaseType_t res = xQueueSend(message_queue_, &message, 0);
    if (res != pdPASS) {
        ESP_LOGW(kTag, "Failed to send melody message, queue full");
        return false;
    }
    
    return true;
}

bool Tune::playMelodyInterruptible(const Melody& melody) {
    if (!is_initialized_) {
        ESP_LOGE(kTag, "Tune not initialized");
        return false;
    }
    
    if (melody.empty()) {
        ESP_LOGW(kTag, "Empty melody");
        return false;
    }
    
    // 复制旋律到临时变量中
    temp_melody_ = melody;
    
    TuneMessage message(MSG_MELODY_INTERRUPTIBLE, &temp_melody_);
    BaseType_t res = xQueueSend(message_queue_, &message, 0);
    if (res != pdPASS) {
        ESP_LOGW(kTag, "Failed to send interruptible melody message, queue full");
        return false;
    }
    
    return true;
}

bool Tune::geigerClick() {
    // if (!is_initialized_) {
    //     ESP_LOGE(kTag, "Tune not initialized");
    //     return false;
    // }
    
    // TuneMessage message(MSG_GEIGER_CLICK);
    // BaseType_t res = xQueueSend(message_queue_, &message, 0);
    // if (res != pdPASS) {
    //     return false;
    // }
    
    // return true;
    if (!isPlaying()) {
        buzzer_->Beep(kGeigerFreq, kGeigerDuration);
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

void Tune::pauseCurrentMelody() {
    if (current_melody_state_ && current_melody_state_->is_active) {
        // 保存当前旋律状态到栈中
        melody_stack_.push(*current_melody_state_);
        ESP_LOGD(kTag, "Current melody paused, saved to stack. Stack size: %d", melody_stack_.size());
        current_melody_state_ = nullptr;
    }
}

void Tune::resumeMelody() {
    if (!melody_stack_.empty()) {
        // 从栈中恢复旋律状态
        MelodyState state = melody_stack_.top();
        melody_stack_.pop();
        
        ESP_LOGD(kTag, "Resuming melody from note %zu/%zu", 
                 state.current_note_index, state.melody.size());
        
        if (state.current_note_index < state.melody.size()) {
            xSemaphoreTake(playing_mutex_, portMAX_DELAY);
            is_playing_ = true;
            xSemaphoreGive(playing_mutex_);
            
            // 从中断的位置继续播放
            for (size_t i = state.current_note_index; i < state.melody.size(); i++) {
                // 检查是否有新消息（中断旋律）
                TuneMessage temp_msg(MSG_STOP);
                if (xQueueReceive(message_queue_, &temp_msg, 0) == pdPASS) {
                    if (temp_msg.type == MSG_MELODY || temp_msg.type == MSG_MELODY_INTERRUPTIBLE) {
                        ESP_LOGD(kTag, "Resumed melody interrupted by new melody");
                        // 保存当前状态并处理新旋律
                        state.current_note_index = i;
                        melody_stack_.push(state);
                        return;
                    } else if (temp_msg.type == MSG_GEIGER_CLICK) {
                        playNote(Note(kGeigerFreq, kGeigerDuration));
                    } else if (temp_msg.type == MSG_STOP) {
                        break;
                    }
                }
                
                playNote(state.melody[i]);
                state.current_note_index = i + 1;
            }
            
            xSemaphoreTake(playing_mutex_, portMAX_DELAY);
            is_playing_ = false;
            xSemaphoreGive(playing_mutex_);
            ESP_LOGD(kTag, "Resumed melody finished");
        }
        
        // 继续检查栈中是否还有其他旋律需要恢复
        if (!melody_stack_.empty()) {
            TuneMessage resume_msg(MSG_RESUME);
            xQueueSend(message_queue_, &resume_msg, 0);
        }
    }
}

void Tune::clearCurrentMelody() {
    current_melody_state_ = nullptr;
    // 清空旋律栈
    while (!melody_stack_.empty()) {
        melody_stack_.pop();
    }
    ESP_LOGD(kTag, "Melody stack cleared");
}

void Tune::stopBuzzer() {
    if (buzzer_) {
        // a reaaaaally short note to act as stop.
        buzzer_->Beep(1, 1);
    }
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
    
    bool tune_play_melody_interruptible(const tune_note_t* notes, size_t note_count) {
        if (!notes || note_count == 0) {
            return false;
        }
        
        Tune::Melody melody;
        melody.reserve(note_count);
        
        for (size_t i = 0; i < note_count; i++) {
            melody.emplace_back(notes[i].frequency, notes[i].duration_ms);
        }
        
        return Tune::getInstance().playMelodyInterruptible(melody);
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