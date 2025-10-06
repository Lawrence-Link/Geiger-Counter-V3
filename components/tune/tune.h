/**
 ******************************************************************************
 * @file        : tune.h
 * @brief       : Tune singleton library for ESP32
 * @author      : Auto Generated
 * @date        : 2024
 ******************************************************************************
 * @copyright   : Copyright (c) 2024
 * @attention   : SPDX-License-Identifier: MIT
 ******************************************************************************
 * @details
 * Tune singleton library that supports:
 * 1. Melody playing with notes (frequency, duration, rests)
 * 2. Geiger counter click sounds (non-blocking)
 * 3. Melody interruption for new melodies with resume capability
 ******************************************************************************
 */
#pragma once
#include "buzzer.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "etl/stack.h"

#ifdef __cplusplus
extern "C" {
#endif

// C 接口声明
typedef struct {
    uint32_t frequency;     // 频率，0表示休止符
    uint32_t duration_ms;   // 持续时间（毫秒）
} tune_note_t;

bool tune_initialize(gpio_num_t gpio_num);
bool tune_play_melody(const tune_note_t* notes, size_t note_count);
bool tune_play_melody_interruptible(const tune_note_t* notes, size_t note_count);
bool tune_geiger_click(void);
void tune_stop(void);
bool tune_is_playing(void);

#ifdef __cplusplus
}
#endif

// C++ 类接口
class Tune {
public:
    // 音符结构体
    struct Note {
        uint32_t frequency;     // 频率，0表示休止符
        uint32_t duration_ms;   // 持续时间（毫秒）
        
        Note(uint32_t freq, uint32_t dur) : frequency(freq), duration_ms(dur) {}
    };
    
    // 旋律类型
    using Melody = std::vector<Note>;
    
    // 消息类型
    enum MessageType {
        MSG_MELODY,
        MSG_MELODY_INTERRUPTIBLE,  // 可中断的临时旋律
        MSG_GEIGER_CLICK,
        MSG_STOP,
        MSG_RESUME                 // 恢复之前中断的旋律
    };
    
    struct TuneMessage {
        MessageType type;
        Melody* melody;  // 使用指针避免拷贝开销
        
        TuneMessage(MessageType t) : type(t), melody(nullptr) {}
        TuneMessage(MessageType t, Melody* m) : type(t), melody(m) {}
    };
    
    // 旋律播放状态
    struct MelodyState {
        Melody melody;
        size_t current_note_index;
        bool is_active;
        
        MelodyState() : current_note_index(0), is_active(false) {}
        MelodyState(const Melody& m) : melody(m), current_note_index(0), is_active(true) {}
    };
    
    // 获取单例实例
    static Tune& getInstance();
    
    // 禁止拷贝构造和赋值
    Tune(const Tune&) = delete;
    Tune& operator=(const Tune&) = delete;
    
    // 初始化（必须在使用前调用）
    bool initialize(gpio_num_t gpio_num = GPIO_NUM_11, 
                   ledc_clk_cfg_t clk_cfg = LEDC_AUTO_CLK,
                   ledc_mode_t speed_mode = LEDC_LOW_SPEED_MODE,
                   ledc_timer_bit_t timer_bit = LEDC_TIMER_13_BIT,
                   ledc_timer_t timer_num = LEDC_TIMER_1,  // 使用 timer 1，避免与 timer 0 冲突
                   ledc_channel_t channel = LEDC_CHANNEL_1);
    
    // 播放旋律（会中断当前播放的旋律，不保存状态）
    bool playMelody(const Melody& melody);
    
    // 播放可中断的临时旋律（播放完后会恢复之前的旋律）
    bool playMelodyInterruptible(const Melody& melody);
    
    // 盖革计数器咔咔声（非阻塞）
    bool geigerClick();
    
    // 停止当前播放
    void stop();
    
    // 检查是否正在播放
    bool isPlaying() const;

private:
    Tune();
    ~Tune();
    
    // FreeRTOS任务相关
    static void taskForwarder(void* pvParameters);
    void tuneTask();
    void stopBuzzer();
    
    // 播放单个音符
    void playNote(const Note& note);
    
    // 旋律栈管理
    void pauseCurrentMelody();
    void resumeMelody();
    void clearCurrentMelody();
    
    // 成员变量
    Buzzer* buzzer_;
    QueueHandle_t message_queue_;
    TaskHandle_t task_handle_;
    SemaphoreHandle_t playing_mutex_;
    bool is_initialized_;
    bool is_playing_;
    
    // 旋律栈管理
    etl::stack<MelodyState, 60> melody_stack_;  // 旋律栈
    MelodyState* current_melody_state_;      // 当前播放的旋律状态
    Melody temp_melody_;                     // 临时存储传入的旋律
    
    // 常量
    static const uint32_t kQueueSize = 10;   // 增加队列大小
    static const uint32_t kStackSize = 4096;
    static const char* kTag;
    
    // 盖革计数器参数
    static const uint32_t kGeigerFreq = 2000;      // 2kHz
    static const uint32_t kGeigerDuration = 10;    // 10ms
};

// 常用音符频率定义（4th octave）
namespace Notes {
    constexpr uint32_t REST = 0;      // 休止符
    constexpr uint32_t C4 = 262;      // Do
    constexpr uint32_t CS4 = 277;     // Do#
    constexpr uint32_t D4 = 294;      // Re
    constexpr uint32_t DS4 = 311;     // Re#
    constexpr uint32_t E4 = 330;      // Mi
    constexpr uint32_t F4 = 349;      // Fa
    constexpr uint32_t FS4 = 370;     // Fa#
    constexpr uint32_t G4 = 392;      // Sol
    constexpr uint32_t GS4 = 415;     // Sol#
    constexpr uint32_t A4 = 440;      // La
    constexpr uint32_t AS4 = 466;     // La#
    constexpr uint32_t B4 = 494;      // Si
    
    // 5th octave
    constexpr uint32_t C5 = 523;
    constexpr uint32_t D5 = 587;
    constexpr uint32_t E5 = 659;
    constexpr uint32_t F5 = 698;
    constexpr uint32_t G5 = 784;
    constexpr uint32_t A5 = 880;
    constexpr uint32_t B5 = 988;
}

// 节拍时长定义（以四分音符为基准，假设 BPM = 120）
namespace Duration {
    constexpr uint32_t WHOLE = 2000;      // 全音符
    constexpr uint32_t HALF = 1000;       // 二分音符
    constexpr uint32_t QUARTER = 500;     // 四分音符
    constexpr uint32_t EIGHTH = 250;      // 八分音符
    constexpr uint32_t SIXTEENTH = 125;   // 十六分音符
}