/*
 * Copyright (C) 2025 Lawrence Link
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
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
#include "core/app/IApplication.h"
#include "core/app/app_system.h"
#include "app_registry.h"

#include <memory>
#include <etl/stack.h>
#include "widgets/histogram/histogram.h"
#include "widgets/brace/brace.h"
#include "widgets/iconButton/iconButton.h"
#include "focus/focus.h"
#include "voltage_pid.hpp"
#include "counter_task.h"
#include "pin_definitions.h"
#include "system_varibles.h"
#include "blinker/Blinker.h"
#include <fastmath.h>

extern VoltagePID voltage_controller;

static const unsigned char image_info_bits[] = {
    0xf0,0xff,0x0f,0xfc,0xff,0x3f,0xde,0xff,0x7b,0x8e,0xff,0x71,0x87,0xff,0xe1,0x03,0xff,0xc0,0x03,0x7e,0xc0,0x01,0x7e,0x80,0x01,0x3c,0x80,0x01,0x3c,0x80,0x01,0x66,0x80,0x01,0xc3,0x80,0xff,0xc3,0xff,0xff,0xe7,0xff,0xff,0xff,0xff,0xff,0xfb,0xff,0xff,0x71,0xff,0xff,0x31,0xff,0xff,0x10,0xfe,0xff,0x10,0xfe,0x7e,0x10,0x74,0xfe,0x10,0x60,0xfc,0xe3,0x3f,0xf0,0xff,0x0f
};
static const unsigned char image_Background_bits[] = {0xfe,0x01,0x00,0x00,0x00,0x00,0x00,0xe0,0xff,0xff,0xff,0x0f,0x00,0x00,0x00,0x00,0x01,0x03,0x00,0x00,0x00,0x00,0x00,0x30,0x00,0x00,0x00,0x18,0x00,0x00,0x00,0x00,0x7d,0x06,0x00,0x00,0x00,0x00,0x00,0x18,0xff,0xb7,0x55,0x31,0x00,0x00,0x00,0x00,0x81,0xfc,0xff,0xff,0xff,0xff,0xff,0x8f,0x00,0x00,0x00,0xe2,0xff,0xff,0xff,0x7f,0x3d,0x01,0x00,0x00,0x00,0x00,0x00,0x40,0xb6,0xea,0xff,0x04,0x00,0x00,0x00,0x80,0x41,0xfe,0xff,0xff,0xaa,0xfe,0xff,0x3f,0x01,0x00,0x00,0xf9,0xff,0xff,0xff,0xab,0x9f,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0xf8,0xff,0x7f,0x02,0x00,0x00,0x00,0x80,0x20,0xff,0xff,0xff,0xff,0x55,0xfd,0x7f,0xfc,0xff,0xff,0x6c,0xff,0xff,0xff,0xb5,0x40,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x06,0x00,0x80,0x01,0x00,0x00,0x00,0x80,0x80,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x03,0x00,0x00,0xff,0xff,0xff,0xff,0xff};

// 7 * 7
static const unsigned char image_SOUND_ON_bits[] = {0x24,0x46,0x57,0x57,0x57,0x46,0x24};
static const unsigned char image_SOUND_OFF_bits[] = {0x04,0x06,0x57,0x27,0x57,0x06,0x04};
// 6 * 7
static const unsigned char image_BELL_bits[] = {0x20,0x18,0x3c,0x3e,0x1f,0x1c,0x12};
// 9 * 7
static const unsigned char image_Alert_bits[] = {0x10,0x00,0x38,0x00,0x28,0x00,0x6c,0x00,0x6c,0x00,0xfe,0x00,0xef,0x01};
// 10 * 6
static const unsigned char image_BAT_FULL_bits[] = {0xff,0x01,0xff,0x03,0xff,0x03,0xff,0x03,0xff,0x03,0xff,0x01};
static const unsigned char image_BAT_75_bits[] = {0xff,0x01,0x3f,0x03,0x3f,0x03,0x3f,0x03,0x3f,0x03,0xff,0x01};
static const unsigned char image_BAT_50_bits[] = {0xff,0x01,0x1f,0x03,0x1f,0x03,0x1f,0x03,0x1f,0x03,0xff,0x01};
static const unsigned char image_BAT_25_bits[] = {0xff,0x01,0x07,0x03,0x07,0x03,0x07,0x03,0x07,0x03,0xff,0x01};
static const unsigned char image_BAT_empty_bits[] = {0xff,0x01,0x01,0x03,0x01,0x03,0x01,0x03,0x01,0x03,0xff,0x01};

// Levels of safety
float safe_until = 300;
float warn_until = 600;
float danger_until = 1000;
float hazardrous_until = 2000;

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdbool.h> // C 中使用 bool 需要这个头文件

// 宏定义单位字符串，方便维护
#define UNIT_STRING       "uSv/h"    // 例子: 单位字符串
#define UNIT_PLACEHOLDER  "-.---uSv/h" // 例子: 占位符（带单位）
#define EMPTY_PLACEHOLDER "-.---"  // 例子: 占位符（无单位）

/**
 * @brief 以万用表风格（固定四位有效数字）将浮点数写入缓冲区。
 * * @param buffer 指向目标字符串缓冲区的指针。
 * @param buffer_size 缓冲区的最大容量。
 * @param value 要格式化的浮点数值（请注意，fabs() 要求 float/double 保持一致）。
 * @param withUnit 是否包含单位字符串（uSv/h）。
 * @return 写入的字符数（不包括终止符 \0），失败时返回负值。
 */

int format_meter_style(char *buffer, size_t buffer_size, float value, bool withUnit) {
    const char* unit_suffix = withUnit ? UNIT_STRING : "";

    if (buffer_size == 0) return 0;

    // 1) Zero placeholder
    if (fabsf(value) < 1e-7f) {
        const char* placeholder = withUnit ? UNIT_PLACEHOLDER : EMPTY_PLACEHOLDER;
        snprintf(buffer, buffer_size, "%s", placeholder);
        return (int)strlen(buffer);
    }

    double d = (double)value;
    double absd = fabs(d);

    char tmp[64];

    // 2) Normal range: fixed-point with up to 3 decimals
    if (absd >= 0.001 && absd < 10000.0) {
        snprintf(tmp, sizeof(tmp), "%.3f", d);

        // Strip trailing zeros and possible trailing dot
        char *pdot = strchr(tmp, '.');
        if (pdot) {
            char *end = tmp + strlen(tmp) - 1;
            while (end > pdot && *end == '0') { *end = '\0'; --end; }
            if (end == pdot) *end = '\0';
        }
    }
    else {
        // 3) Scientific notation for very small / large
        snprintf(tmp, sizeof(tmp), "%.3g", d);
    }

    snprintf(buffer, buffer_size, "%s%s", tmp, unit_suffix);
    return (int)strlen(buffer);
}

// --- USER DEFINED APP: A Geiger counter UI demo ---
class APP_COUNTER: public IApplication {
private:
    PixelUI& m_ui;
    Histogram histogram;
    Brace brace;
    FocusManager m_focusMan;
    IconButton icon_battery;
    IconButton icon_sounding;
    IconButton icon_alarm;
    // State machine for loading animation sequence
    enum class LoadState {
        INIT,          // start
        BRACE_LOADING, // execute brace.onLoad()
        WAIT_HISTO,    // wait 300ms
        HISTO_LOADING, // execute histogram.onLoad()
        DONE           // all done
    } loadState = LoadState::INIT;
    uint32_t state_timestamp = 0;  // record time when entering a state
    bool first_time = false;
    // animation related variables
    int32_t anim_mark_m = 0;
    int32_t anim_bg = 0;
    int32_t anim_status_x = -27;
    int32_t anim_clock_y = 0;
    char print_buffer[24];
    uint32_t timestamp_prev;
    uint32_t timestamp_now;
    Blinker blinker;

    float current_cpm = 0;

public:
    APP_COUNTER(PixelUI& ui) : 
    m_ui(ui), 
    histogram(ui), 
    brace(ui), 
    m_focusMan(ui), 
    icon_battery(ui),
    icon_sounding(ui),
    icon_alarm(ui),
    blinker(ui, 100)
    {}
    void onEnter(ExitCallback cb) override {
        IApplication::onEnter(cb);
        m_ui.setContinousDraw(true);
        // HISTOGRAM
        histogram.setCoordinate(97,54);
        histogram.setMargin(56,18);
        histogram.setFocusBox(FocusBox(70,46,55,17));
        histogram.setExpand(EXPAND_BASE::BOTTOM_RIGHT, 76, 63);
        // BRACE 
        brace.setCoordinate(31,54);
        brace.setFocusBox(FocusBox(4, 46, 55, 17));
        brace.setMargin(56,18);
        brace.setDrawContentFunction([this]() { braceContent(); });
        // icon battery
        icon_battery.setSource(image_BAT_75_bits);
        icon_battery.setMargin(10, 6);
        icon_battery.setCoordinate(12, 2);
        // icon sounding
        icon_sounding.setSource(image_SOUND_OFF_bits);
        icon_sounding.setMargin(7, 7);
        icon_sounding.setCoordinate(26, 1);
        
        // icon alarm
        icon_alarm.setSource(image_BELL_bits);
        icon_alarm.setMargin(6, 7);
        icon_alarm.setCoordinate(36, 1);

        // Adding widgets to focus manager, enabling cursor navigation
        m_focusMan.addWidget(&brace);
        m_focusMan.addWidget(&histogram);  

        timestamp_prev = timestamp_now = m_ui.getCurrentTime();

        loadState = LoadState::INIT;
        first_time = false;

        voltage_controller.startTask();
        counter_task_config_t tube_conf = {
            .gpio_num = PIN_PULSE_IN
        };
        start_counter_task(&tube_conf);
    }

    void braceContent() {
        U8G2& u8g2 = m_ui.getU8G2();
        u8g2.setFont(u8g2_font_5x7_tr);
        format_meter_style(print_buffer, sizeof(print_buffer), histogram.getMaxValue(), false);
        u8g2.drawStr(30, 54, print_buffer);
        u8g2.drawStr(31, 61, "uSv/h");
        u8g2.drawRBox(8, 50, 20, 10, 2);
        u8g2.setDrawColor(0);
        u8g2.drawStr(11, 58, "Max");
        u8g2.setDrawColor(1);
    }

    void drawLabel( bool iscalib ) {
        U8G2& u8g2 = m_ui.getU8G2();
        if (!iscalib) {
            u8g2.setFont(u8g2_font_5x7_tr);
            
            if (current_cpm < safe_until) {
                u8g2.drawStr(5, 42, "SAFE");
                u8g2.setClipWindow(29,36,128,42);
                u8g2.drawStr(anim_status_x, 42, "Low Radiation");
                u8g2.setMaxClipWindow();
                blinker.stopOnVisible();
            }
            else if (current_cpm < warn_until) {
                u8g2.drawStr(5, 42, "WARN");
                u8g2.setClipWindow(29,36,128,42);
                u8g2.drawStr(anim_status_x, 42, "RISING LEVEL");
                u8g2.setMaxClipWindow();
                blinker.set_interval(500);
                blinker.start();
            }
            else if (current_cpm < danger_until) {
                u8g2.drawStr(5, 42, "DNGR");
                u8g2.setClipWindow(29,36,128,42);
                u8g2.drawStr(anim_status_x, 42, "UNSAFE DOSE");
                u8g2.setMaxClipWindow();
                blinker.set_interval(300);
                blinker.start();
            }
            else if (current_cpm < hazardrous_until) {
                u8g2.drawStr(5, 42, "HZDR");
                u8g2.setClipWindow(29,36,128,42);
                u8g2.drawStr(anim_status_x, 42, "SEVERE THREAT");
                u8g2.setMaxClipWindow();
                blinker.set_interval(100);
                blinker.start();
            }

            u8g2.setDrawColor(2);
            u8g2.drawBox(3, 35, anim_mark_m, 8);
            u8g2.setDrawColor(1);
            

        } else {
            u8g2.setFont(u8g2_font_5x7_tr);
            u8g2.drawStr(5, 42, "CALI");
            u8g2.setClipWindow(29,36,128,42);
            u8g2.drawStr(anim_status_x, 42, "PLEASE WAIT");
            u8g2.setMaxClipWindow();
            u8g2.setDrawColor(2);
            u8g2.drawBox(3, 35, anim_mark_m, 8);
            u8g2.setDrawColor(1);
            u8g2.setFont(u8g2_font_profont17_tr);
        }
    }

    void draw() override {
        timestamp_now = m_ui.getCurrentTime();
        if (!first_time) {
            m_ui.animate(anim_mark_m, 23, 300, EasingType::EASE_OUT_QUAD, PROTECTION::PROTECTED);
            m_ui.animate(anim_bg, 128, 500, EasingType::EASE_IN_OUT_CUBIC, PROTECTION::PROTECTED);
            m_ui.animate(anim_clock_y, 8, 200, EasingType::EASE_OUT_CUBIC, PROTECTION::PROTECTED);
            
            // the state machine start from its initial state
            loadState = LoadState::BRACE_LOADING;
            state_timestamp = m_ui.getCurrentTime();
            first_time = true;
        }
        // state machine
        switch (loadState) {
            case LoadState::BRACE_LOADING:
                brace.onLoad();
                icon_battery.onLoad();
                loadState = LoadState::WAIT_HISTO;
                state_timestamp = m_ui.getCurrentTime(); // record when was the state entered
                break;
            case LoadState::WAIT_HISTO:
                if (m_ui.getCurrentTime() - state_timestamp >= 80) {
                    loadState = LoadState::HISTO_LOADING;
                }
                break;
            case LoadState::HISTO_LOADING:
                histogram.onLoad();
                icon_sounding.onLoad();
                icon_alarm.onLoad();
                loadState = LoadState::DONE;
                m_ui.animate(anim_status_x, 29, 450, EasingType::EASE_OUT_CUBIC, PROTECTION::PROTECTED);
                break;
            case LoadState::DONE:
                // Do nothing, wait for exit
                break;
            default:
                break;
        }
        // UI drawing
        U8G2& u8g2 = m_ui.getU8G2();
        u8g2.setClipWindow(0,7,anim_bg,18);
        u8g2.drawXBM(0, 7, 128, 10, image_Background_bits);
        u8g2.setMaxClipWindow();
        
        u8g2.setFont(u8g2_font_profont17_tr);
        current_cpm = get_current_cpm();
        // snprintf(print_buffer, sizeof(print_buffer), "%.4guSv/h", current_cpm * SystemConf::getInstance().read_conf_tube_convertion_coefficient());
        format_meter_style(print_buffer, sizeof(print_buffer), current_cpm * SystemConf::getInstance().read_conf_tube_convertion_coefficient(), true);
        u8g2.drawStr(3, 31, print_buffer);

        blinker.update();
        if (timestamp_now - timestamp_prev >= 1000){
            if (!is_startup_mode()){
                timestamp_prev = timestamp_now;
                histogram.addData(current_cpm * SystemConf::getInstance().read_conf_tube_convertion_coefficient());
                blinker.stopOnVisible();
            } else {
                blinker.start();
            }
        }

        if (blinker.is_visible()) {
            if (is_startup_mode()) {drawLabel( true );}
            else                 {drawLabel( false);}
        }

        u8g2.setFont(u8g2_font_5x7_tr);
        uint16_t volt = voltage_controller.getVoltage();
        
        snprintf(print_buffer, sizeof(print_buffer), "%dV", volt);
        u8g2.drawStr(105, 42, print_buffer);

        if (abs(volt - voltage_controller.getTargetVolt()) < 10) {
            u8g2.setDrawColor(2);
            u8g2.drawBox(104, 35, 21, 8);
        }

        u8g2.setDrawColor(1);
        
        u8g2.setFont(u8g2_font_5x7_tr);
        u8g2.drawStr(97, anim_clock_y, "07:23P");

        // draw widgets
        icon_sounding.draw();
        icon_alarm.draw();
        icon_battery.draw();
        brace.draw();

        if (histogram.isExpanded()) {
            u8g2.clearBuffer();
            u8g2.drawStr(3, 10, "<STATS>");
            u8g2.drawStr(3, 20, "Max:");
            snprintf(print_buffer, sizeof(print_buffer), "%.3gusv/h", histogram.getMaxValue());
            u8g2.drawStr(3, 30, print_buffer);
            u8g2.drawStr(3, 40, "Avg:");
            snprintf(print_buffer, sizeof(print_buffer), "%.3gusv/h", histogram.getAverageValue());
            u8g2.drawStr(3, 50, print_buffer);
        }
        histogram.draw();
        m_focusMan.draw();
    }
    bool handleInput(InputEvent event) override {
        // Check if a widget has taken over input control
        IWidget* activeWidget = m_focusMan.getActiveWidget();
        if (activeWidget) {
            // If so, pass the event to that widget
            if (activeWidget->handleEvent(event)) {
                // If the widget returns true, it means it has finished processing and control is handed back to the FocusManager
                m_focusMan.clearActiveWidget();
            }
            return true; // Event has been handled, return true
        }
        // No widget has taken over input, execute the original focus management logic
        if (event == InputEvent::BACK) {
            requestExit();
        } else if (event == InputEvent::RIGHT) {
            m_focusMan.moveNext();
        } else if (event == InputEvent::LEFT) {
            m_focusMan.movePrev();
        } else if (event == InputEvent::SELECT) {
            m_focusMan.selectCurrent();
        }
        return true;
    }

    void onExit() {
        voltage_controller.stop();
        stop_counter_task();
        m_ui.clearAllAnimations();
        m_ui.setContinousDraw(false);
        m_ui.markFading();
    }
};
AppItem counter_app{
    .title = "COUNTER",
    .bitmap = image_info_bits,
    
    .createApp = [](PixelUI& ui) -> std::unique_ptr<IApplication> { 
        return std::make_unique<APP_COUNTER>(ui); 
    },
    
    .type = MenuItemType::App,
    .order = 0
};