/*
 * Copyright (C) 2025 Lawrence Link
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "core/app/IApplication.h"
#include "core/app/app_system.h"
#include "app_registry.h"
#include <memory>
#include "widgets/histogram/histogram.h"
#include "widgets/curve_chart/curve_chart.h"
#include "widgets/brace/brace.h"
#include "widgets/icon_button/icon_button.h"
#include "focus/focus.h"
#include "voltage_pid.hpp"
#include "counter_task.h"
#include "time_module.h"
#include "pin_definitions.h"
#include "system_nvs_varibles.h"
#include "blinker/Blinker.h"
#include <fastmath.h>
#include "i2c.h" 
#include "core/coroutine/Coroutine.h"
#include "calc/text_centering_calc/text_centering_calculator.h"
#include "led.h"
#include "tune.h"
#include "esp_log.h"
#include "resources.h"

extern VoltagePID voltage_controller;
extern int battery_percentage;
extern bool pulse_marker;

// Radiation level thresholds (cpm or converted units, depending on usage)
static int32_t cpm_warn_threshold = 300;
static int32_t cpm_dngr_threshold = 600;
static int32_t cpm_hzdr_threshold = 1000;
static bool use_cpm;

// Macro definitions for unit strings
#define UNIT_USV "uSv/h"
#define UNIT_CPM "CPM"
#define UNIT_PLACEHOLDER_USV  "-.---uSv/h"
#define UNIT_PLACEHOLDER_CPM  "-.---CPM"
#define EMPTY_PLACEHOLDER     "-.---"


class APP_COUNTER: public IApplication {
private:
    PixelUI& m_ui;
    Histogram histogram;
    Brace brace;
    IconButton icon_battery;
    IconButton icon_sounding;
    IconButton icon_alarm;
    
    // State machine for the initial loading animation sequence
    enum class LoadState {
        INIT,          // Initial state
        BRACE_LOADING, // Execute brace.onLoad()
        WAIT_HISTO,    // Wait for a short delay
        HISTO_LOADING, // Execute histogram.onLoad()
        DONE           // Loading complete
    } loadState = LoadState::INIT;
    
    uint32_t state_timestamp = 0;  // Time of entering the current state
    bool first_time = false;
    
    // Animation variables
    int32_t anim_mark_m = 0;      // Animation for the color bar width
    int32_t anim_bg = 0;          // Animation for the background clipping width
    int32_t anim_status_x = -27;  // Animation for the status text horizontal position
    int32_t anim_clock_y = 0;     // Animation for the clock vertical position
    
    // Brace 页面滚动相关
    enum class BracePage {
        MAX = 0,
        AVG = 1
    };
    
    BracePage currentBracePage = BracePage::MAX;
    BracePage targetBracePage = BracePage::MAX;
    int32_t anim_brace_y = 0;  // Brace 内容的垂直滚动动画
    bool brace_animating = false;
    
    char print_buffer[24];        // Buffer for formatted strings
    
    uint32_t timestamp_prev;
    uint32_t timestamp_now;
    
    Blinker blinker_description_bar;
    
    float current_cpm = 0.0f;
    struct tm timeinfo;
    bool tm_valid;

    Coroutine coroutine_anim;
    Coroutine coroutine_alarm;

    bool requireAlertSound = false;
    bool en_dosage_alert;
    bool en_click = false;
    int bracePageCnt = 0;
    Tune& tune = Tune::getInstance();

public:
    APP_COUNTER(PixelUI& ui, void* param) : 
    m_ui(ui), 
    histogram(ui, 69, 45, 56, 18, 76, 63, EXPAND_BASE::BOTTOM_RIGHT), 
    brace(ui, 3, 45, 56, 18), 
    icon_battery(ui, 12, 2, 10, 6),
    icon_sounding(ui, 26, 1, 7, 7),
    icon_alarm(ui, 37, 1, 7, 7),
    blinker_description_bar(ui, 100),
    coroutine_anim([this](CoroutineContext& ctx) 
    {
        CORO_BEGIN(ctx);
            brace.onLoad();
        CORO_DELAY(ctx, m_ui, 80, 100);
            histogram.onLoad();
            icon_battery.onLoad();
            icon_sounding.onLoad();
            icon_alarm.onLoad();
            m_ui.animate(anim_status_x, 29, 450, EasingType::EASE_OUT_CUBIC, PROTECTION::PROTECTED);
        CORO_END(ctx);
    }
    ),

    coroutine_alarm([this](CoroutineContext& ctx) {
        CORO_BEGIN(ctx);
    
        while (true) {
            while (!ctx.localData[0]) {
                CORO_YIELD(ctx, __LINE__);
            }
            ctx.localData[0] = 0;
            if (en_dosage_alert) {
                tune.playMelodyInterruptible(sos);
            }
            CORO_DELAY(ctx, m_ui, 3500, __LINE__);
        }
        CORO_END(ctx);
    })

    {}
    
    // Called when the application is started
    void onEnter(ExitCallback cb) override ;
    
    void braceCallback() ;
    
    // Drawing function for the Brace widget content (Max/Avg value with scrolling)
    void braceContent() ;
    
    void onResume() override ;
    
    // Draws the radiation level status text and color bar
    void drawLabel(int state) ;
    
    void draw() override ;

    bool handleInput(InputEvent event) override ;
    // Called when the application is exited
    void onExit() override ;
};