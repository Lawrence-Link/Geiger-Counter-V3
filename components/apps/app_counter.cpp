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
#include "core/app/IApplication.h"
#include "core/app/app_system.h"
#include "app_registry.h"
#include <memory>
#include <etl/stack.h>
#include "widgets/histogram/histogram.h"
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
#include "i2c.h" // for the handles
#include "core/coroutine/Coroutine.h"
#include "led.h"
#include "tune.h"
#include "esp_log.h"

extern VoltagePID voltage_controller;
extern int battery_percentage;

// XBM data for application icon (info)
__attribute__((aligned(4)))
static const unsigned char image_counter_bits[] = {
    0xf0,0xff,0x0f,0xfc,0xff,0x3f,0xde,0xff,0x7b,0x8e,0xff,0x71,0x87,0xff,0xe1,0x03,0xff,0xc0,0x03,0x7e,0xc0,0x01,0x7e,0x80,0x01,0x3c,0x80,0x01,0x3c,0x80,0x01,0x66,0x80,0x01,0xc3,0x80,0xff,0xc3,0xff,0xff,0xe7,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xc3,0xff,0xff,0xc3,0xff,0xff,0x00,0xff,0xff,0x00,0xff,0x7e,0x00,0x7e,0xfe,0x00,0x7f,0xfc,0xc3,0x3f,0xf0,0xff,0x0f
};

// XBM data for main background
__attribute__((aligned(4)))
static const unsigned char image_Background_bits[] = {0xfe,0x01,0x00,0x00,0x00,0x00,0x00,0xe0,0xff,0xff,0xff,0x0f,0x00,0x00,0x00,0x00,0x01,0x03,0x00,0x00,0x00,0x00,0x00,0x30,0x00,0x00,0x00,0x18,0x00,0x00,0x00,0x00,0x7d,0x06,0x00,0x00,0x00,0x00,0x00,0x18,0xff,0xb7,0x55,0x31,0x00,0x00,0x00,0x00,0x81,0xfc,0xff,0xff,0xff,0xff,0xff,0x8f,0x00,0x00,0x00,0xe2,0xff,0xff,0xff,0x7f,0x3d,0x01,0x00,0x00,0x00,0x00,0x00,0x40,0xb6,0xea,0xff,0x04,0x00,0x00,0x00,0x80,0x41,0xfe,0xff,0xff,0xaa,0xfe,0xff,0x3f,0x01,0x00,0x00,0xf9,0xff,0xff,0xff,0xab,0x9f,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0xf8,0xff,0x7f,0x02,0x00,0x00,0x00,0x80,0x20,0xff,0xff,0xff,0xff,0x55,0xfd,0x7f,0xfc,0xff,0xff,0x6c,0xff,0xff,0xff,0xb5,0x40,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x06,0x00,0x80,0x01,0x00,0x00,0x00,0x80,0x80,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x03,0x00,0x00,0xff,0xff,0xff,0xff,0xff};

// XBM data for icons (7x7)
__attribute__((aligned(4)))
static const unsigned char image_SOUND_ON_bits[] = {0x24,0x46,0x57,0x57,0x57,0x46,0x24};
__attribute__((aligned(4)))
static const unsigned char image_SOUND_OFF_bits[] = {0x04,0x06,0x57,0x27,0x57,0x06,0x04};

// XBM data for BELL icon (6x7)
__attribute__((aligned(4)))
static const unsigned char image_BELL_bits[] = {0x20,0x18,0x3c,0x3e,0x1f,0x1c,0x12};
__attribute__((aligned(4)))
static const uint8_t image_BELL_OFF_bits[] = {0x21,0x1a,0x3c,0x3e,0x1f,0x3c,0x12};


// XBM data for battery icons (10x6)
__attribute__((aligned(4)))
static const unsigned char image_BAT_FULL_bits[] = {0xff,0x01,0xff,0x03,0xff,0x03,0xff,0x03,0xff,0x03,0xff,0x01};
__attribute__((aligned(4)))
static const unsigned char image_BAT_75_bits[] = {0xff,0x01,0x3f,0x03,0x3f,0x03,0x3f,0x03,0x3f,0x03,0xff,0x01};
__attribute__((aligned(4)))
static const unsigned char image_BAT_50_bits[] = {0xff,0x01,0x1f,0x03,0x1f,0x03,0x1f,0x03,0x1f,0x03,0xff,0x01};
__attribute__((aligned(4)))
static const unsigned char image_BAT_25_bits[] = {0xff,0x01,0x07,0x03,0x07,0x03,0x07,0x03,0x07,0x03,0xff,0x01};
__attribute__((aligned(4)))
static const unsigned char image_BAT_empty_bits[] = {0xff,0x01,0x01,0x03,0x01,0x03,0x01,0x03,0x01,0x03,0xff,0x01};


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

Tune::Melody sos = {
    // S: (···)
    {Notes::B5, 70},
    {Notes::REST, 70},    
    {Notes::B5, 70},      
    {Notes::REST, 70},    
    {Notes::B5, 70},      
    {Notes::REST, 250},    
    
    // O: (---)
    {Notes::B5, 250},      
    {Notes::REST, 70},    
    {Notes::B5, 250},      
    {Notes::REST, 70},    
    {Notes::B5, 250},      
    {Notes::REST, 250},    
    
    // S: (···)
    {Notes::B5, 70},      
    {Notes::REST, 70},    
    {Notes::B5, 70},      
    {Notes::REST, 70},    
    {Notes::B5, 70},      
};

/**
 * @brief Formats a floating-point number into a string in a meter-style format (up to 4 significant digits).
 *
 * @param buffer Pointer to the destination string buffer.
 * @param buffer_size Maximum capacity of the buffer.
 * @param value The float value to format.
 * @param unit  Optional unit string ("uSv/h", "CPM", or NULL for no unit).
 * @return The number of characters written (excluding the null terminator), or a negative value on failure.
 */
int format_meter_style(char *buffer, size_t buffer_size, float value, const char *unit) {
    if (buffer_size == 0) return 0;
    
    bool withUnit = (unit != NULL && unit[0] != '\0');
    
    // 1) Zero/near-zero value displays a placeholder
    if (fabsf(value) < 1e-7f) {
        const char *placeholder;
        if (withUnit) {
            if (strcmp(unit, UNIT_CPM) == 0)
                placeholder = UNIT_PLACEHOLDER_CPM;
            else
                placeholder = UNIT_PLACEHOLDER_USV;
        } else {
            placeholder = EMPTY_PLACEHOLDER;
        }
        snprintf(buffer, buffer_size, "%s", placeholder);
        return (int)strlen(buffer);
    }
    
    double d = (double)value;
    double absd = fabs(d);
    char tmp[64];
    
    // 2) Normal range: fixed-point with up to 3 decimal places
    if (absd >= 0.001 && absd < 10000.0) {
        snprintf(tmp, sizeof(tmp), "%.3f", d);
        // Strip trailing zeros and possible trailing dot
        char *pdot = strchr(tmp, '.');
        if (pdot) {
            char *end = tmp + strlen(tmp) - 1;
            while (end > pdot && *end == '0') { *end = '\0'; --end; }
            if (end == pdot) *end = '\0'; // Remove trailing dot if all decimals were zero
        }
    }
    else {
        // 3) Scientific notation for very small or very large values
        snprintf(tmp, sizeof(tmp), "%.3g", d);
    }
    
    // Combine number and unit suffix
    if (withUnit)
        snprintf(buffer, buffer_size, "%s%s", tmp, unit);
    else
        snprintf(buffer, buffer_size, "%s", tmp);
    
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
    Blinker blinker_calibration_icon;
    
    float current_cpm = 0;
    struct tm timeinfo;
    bool tm_valid;
    
    std::shared_ptr<Coroutine> animationCoroutine_;
    std::shared_ptr<Coroutine> alertSchedulerCoroutine_; // localData[0] is the indicator of an alert.

    bool requireAlertSound = false;
    bool en_dosage_alert;
    bool en_click = false;
    Tune& tune = Tune::getInstance();

    void alert_scheduler_coroutine_body(CoroutineContext& ctx, PixelUI& ui) {
        CORO_BEGIN(ctx);
    
        while (true) {
            while (!ctx.localData[0]) {
                CORO_YIELD(ctx, __LINE__);
            }
            ctx.localData[0] = 0;
            if (en_dosage_alert) tune.playMelodyInterruptible(sos);
            CORO_DELAY(ctx, ui, 3500, __LINE__);
        }
        CORO_END(ctx);
    }

    void animation_coroutine_body(CoroutineContext& ctx, PixelUI& ui) 
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

public:
    APP_COUNTER(PixelUI& ui) : 
    m_ui(ui), 
    histogram(ui), 
    brace(ui), 
    m_focusMan(ui), 
    icon_battery(ui),
    icon_sounding(ui),
    icon_alarm(ui),
    blinker_description_bar(ui, 100),
    blinker_calibration_icon(ui, 100)
    {}
    
    // Called when the application is started
    void onEnter(ExitCallback cb) override {
        IApplication::onEnter(cb);
        
        auto& syscfg = SystemConf::getInstance();
        cpm_warn_threshold = syscfg.read_conf_warn_threshold();
        cpm_dngr_threshold = syscfg.read_conf_dngr_threshold();
        cpm_hzdr_threshold = syscfg.read_conf_hzdr_threshold();
        use_cpm = syscfg.read_conf_use_cpm();
        en_dosage_alert = syscfg.read_conf_enable_alert();
        en_click = syscfg.read_conf_enable_geiger_click();

        m_ui.setContinousDraw(true);
        pcf8563_get_time(pcf8563_dev, &timeinfo, &tm_valid);
        
        // Initialize and configure widgets
        // HISTOGRAM
        histogram.setPosition(97,54);
        histogram.setSize(56,18);
        histogram.setFocusBox(FocusBox(70,46,55,17));
        histogram.setExpand(EXPAND_BASE::BOTTOM_RIGHT, 76, 63);
        
        // BRACE 
        brace.setPosition(31,54);
        brace.setFocusBox(FocusBox(4, 46, 55, 17));
        brace.setSize(56,18);
        // Set a lambda function to draw the brace content (Max value)
        brace.setDrawContentFunction([this]() { braceContent(); });
        brace.setCallback([this](){braceCallback();});
        
        // 初始化 Brace 页面状态
        currentBracePage = BracePage::MAX;
        targetBracePage = BracePage::MAX;
        anim_brace_y = 0;
        brace_animating = false;
        
        // ICON: Battery
        icon_battery.setSource(image_BAT_75_bits);
        icon_battery.setSize(10, 6);
        icon_battery.setPosition(12, 2);
        
        // ICON: Sounding
        if (en_click) icon_sounding.setSource(image_SOUND_ON_bits);
        else          icon_sounding.setSource(image_SOUND_OFF_bits);
        icon_sounding.setSize(7, 7);
        icon_sounding.setPosition(26, 1);
        
        // ICON: Alarm
        if (en_dosage_alert) icon_alarm.setSource(image_BELL_bits);
        else                 icon_alarm.setSource(image_BELL_OFF_bits);
        icon_alarm.setSize(7, 7);
        icon_alarm.setPosition(36, 1);
        
        // Add widgets to focus manager for navigation
        m_focusMan.addWidget(&brace);
        m_focusMan.addWidget(&histogram);  
        
        timestamp_prev = timestamp_now = m_ui.getCurrentTime();
        
        animationCoroutine_ = std::make_shared<Coroutine>(
            std::bind(&APP_COUNTER::animation_coroutine_body, this, std::placeholders::_1, std::placeholders::_2),
            m_ui
        );

        alertSchedulerCoroutine_ = std::make_shared<Coroutine>(
            std::bind(&APP_COUNTER::alert_scheduler_coroutine_body, this, std::placeholders::_1, std::placeholders::_2),
            m_ui
        );

        m_ui.addCoroutine(animationCoroutine_);
        m_ui.addCoroutine(alertSchedulerCoroutine_); // 警报调度
        
        first_time = false;
        
        // Start hardware-related tasks
        voltage_controller.startTask();
        counter_task_config_t tube_conf = {
            .gpio_num = PIN_PULSE_IN
        };
        start_counter_task(&tube_conf);
    }
    
    int bracePageCnt = 0;
    
    void braceCallback() {
        if (brace_animating) return;  // 如果正在动画中，忽略输入
        
        // 计算下一个页面 (MAX -> AVG -> MAX)
        targetBracePage = (currentBracePage == BracePage::MAX) ? BracePage::AVG : BracePage::MAX;
        
        // 启动向上滚动动画
        const int PAGE_HEIGHT = 18;
        anim_brace_y = 0;
        m_ui.animate(anim_brace_y, PAGE_HEIGHT, 300, EasingType::EASE_IN_OUT_CUBIC, PROTECTION::PROTECTED);
        brace_animating = true;
    }
    
    // Drawing function for the Brace widget content (Max/Avg value with scrolling)
    void braceContent() {
        U8G2& u8g2 = m_ui.getU8G2();
        u8g2.setFont(u8g2_font_5x7_tr);
        
        const int PAGE_HEIGHT = 18;
        
        auto drawPage = [&](BracePage page, int y_base) {
            const char* label = nullptr;
            float value = 0.0f;
            
            switch (page) {
                case BracePage::MAX:
                    label = "Max";
                    value = histogram.getMaxValue();
                    break;
                case BracePage::AVG:
                    label = "Avg";
                    value = histogram.getAverageValue();
                    break;
            }
            
            // 绘制数值
            if (!use_cpm) {
                format_meter_style(print_buffer, sizeof(print_buffer), value, NULL);
                u8g2.drawStr(30, y_base - 4, print_buffer);
                u8g2.drawStr(31, y_base + 3, "uSv/h");
            } else {
                snprintf(print_buffer, sizeof(print_buffer), "%d", (int)value);
                u8g2.drawStr(30, y_base - 4, print_buffer);
                u8g2.drawStr(31, y_base + 3, "CPM");
            }
            
            // 绘制标签框
            u8g2.drawRBox(8, y_base - 8, 20, 10, 2);
            u8g2.setDrawColor(0);
            u8g2.drawStr(11, y_base, label);
            u8g2.setDrawColor(1);
        };
        
        // 根据动画进度绘制页面
        int current_y = 58 + anim_brace_y;  // 当前页面的 Y 坐标
        int next_y = 58 + anim_brace_y - PAGE_HEIGHT;  // 下一个页面的 Y 坐标（从下方进入）
        
        // 绘制当前页面（向上移出）
        if (current_y >= 45 && current_y <= 70) {  // 在可视范围内
            drawPage(currentBracePage, current_y);
        }
        
        // 绘制目标页面（从下方进入）
        if (next_y >= 45 && next_y <= 70) {  // 在可视范围内
            drawPage(targetBracePage, next_y);
        }
    }
    
    void onResume() override {
        m_ui.setContinousDraw(true);
    }
    
    // Draws the radiation level status text and color bar
    void drawLabel() {
        U8G2& u8g2 = m_ui.getU8G2();
        u8g2.setFont(u8g2_font_5x7_tr);
        
        if (current_cpm > 0) {
            // Determine status text based on radiation thresholds
            if (current_cpm < cpm_warn_threshold) {
                u8g2.drawStr(5, 42, "SAFE");
                u8g2.setClipWindow(29,36,128,42);
                u8g2.drawStr(anim_status_x, 42, "Low Radiation");
                u8g2.setMaxClipWindow();
                blinker_description_bar.stopOnVisible();
            }
            else if (current_cpm < cpm_dngr_threshold) {
                u8g2.drawStr(5, 42, "WARN");
                u8g2.setClipWindow(29,36,128,42);
                u8g2.drawStr(anim_status_x, 42, "RISING LEVEL");
                u8g2.setMaxClipWindow();
                blinker_description_bar.set_interval(500);
                blinker_description_bar.start();
            }
            else if (current_cpm < cpm_hzdr_threshold) {
                alertSchedulerCoroutine_->getContext().localData[0] = 1;

                u8g2.drawStr(5, 42, "DNGR");
                u8g2.setClipWindow(29,36,128,42);
                u8g2.drawStr(anim_status_x, 42, "UNSAFE DOSE");
                u8g2.setMaxClipWindow();
                blinker_description_bar.set_interval(300);
                blinker_description_bar.start();
            }
            else {
                alertSchedulerCoroutine_->getContext().localData[0] = 1;
                u8g2.drawStr(5, 42, "HZDR");
                u8g2.setClipWindow(29,36,128,42);
                u8g2.drawStr(anim_status_x, 42, "SEVERE THREAT");
                u8g2.setMaxClipWindow();
                blinker_description_bar.set_interval(100);
                blinker_description_bar.start();
            }
        } else {
            // Pending/initial state
            u8g2.drawStr(5, 42, "PEND");
            u8g2.setClipWindow(29,36,128,42);
            u8g2.drawStr(anim_status_x, 42, "PLEASE WAIT");
            u8g2.setMaxClipWindow();
            blinker_description_bar.set_interval(180);
            blinker_description_bar.start();
        }
        
        // Draw the colored status bar (using draw color 2, typically inverse/toggle mode)
        u8g2.setDrawColor(2);
        u8g2.drawBox(3, 35, anim_mark_m, 8);
        u8g2.setDrawColor(1);    
    }
    
    void draw() override {
        timestamp_now = m_ui.getCurrentTime();
        
        // Initial setup and animation start
        if (!first_time) {
            m_ui.animate(anim_mark_m, 23, 300, EasingType::EASE_OUT_QUAD, PROTECTION::PROTECTED);
            m_ui.animate(anim_bg, 128, 500, EasingType::EASE_IN_OUT_CUBIC, PROTECTION::PROTECTED);
            m_ui.animate(anim_clock_y, 8, 200, EasingType::EASE_OUT_CUBIC, PROTECTION::PROTECTED);
            
            blinker_description_bar.stopOnVisible();
            // Start the loading animation state machine
            state_timestamp = m_ui.getCurrentTime();
            first_time = true;
        }
        
        if (brace_animating && anim_brace_y >= 17) {  // animation near complete
            // reset state
            currentBracePage = targetBracePage;
            anim_brace_y = 0;
            brace_animating = false;
        }
        
        // --- UI Drawing ---
        U8G2& u8g2 = m_ui.getU8G2();
        
        // Draw background with animation clipping
        u8g2.setClipWindow(0,7,anim_bg,18);
        u8g2.drawXBM(0, 7, 128, 10, image_Background_bits);
        u8g2.setMaxClipWindow();
        
        // Draw the main radiation reading
        u8g2.setFont(u8g2_font_profont17_tr);
        current_cpm = get_current_cpm();
        
        // Format and draw the value (CPM * conversion coefficient)
        if (!use_cpm) {
            format_meter_style(print_buffer, sizeof(print_buffer), current_cpm * SystemConf::getInstance().read_conf_tube_convertion_coefficient(), UNIT_USV);
        } else {
            snprintf(print_buffer, sizeof(print_buffer), "%dCPM", (int)current_cpm);
        }
        u8g2.drawStr(3, 31, print_buffer);
        
        blinker_description_bar.update();
        blinker_calibration_icon.update();
        
        // Update histogram data every second if not in startup mode
        if (timestamp_now - timestamp_prev >= 1000){
            if (!is_startup_mode()){
                timestamp_prev = timestamp_now;
                if (!use_cpm) {
                    histogram.addData(current_cpm * SystemConf::getInstance().read_conf_tube_convertion_coefficient());
                } else {
                    histogram.addData(current_cpm);
                }
                blinker_calibration_icon.stop();
            } else {
                // If in startup mode, display the calibration blinker
                blinker_calibration_icon.start();
            }
            
            const unsigned char* bat_source = nullptr;
            if (battery_percentage >= 75) {
                bat_source = image_BAT_FULL_bits; // 100% or 75%+
            } else if (battery_percentage >= 50) {
                bat_source = image_BAT_75_bits;   // 50% - 74%
            } else if (battery_percentage >= 25) {
                bat_source = image_BAT_50_bits;   // 25% - 49%
            } else if (battery_percentage > 0) {
                bat_source = image_BAT_25_bits;   // 1% - 24%
            } else {
                bat_source = image_BAT_empty_bits; // 0% or empty
            }
            icon_battery.setSource(bat_source);
            
            pcf8563_get_time(pcf8563_dev, &timeinfo, &tm_valid);
        }
        
        // Draw status label if the blinker is visible (for blinking effect)
        if (blinker_description_bar.is_visible()) {
            drawLabel();
        }
        
        // Draw "CAL" text if the calibration blinker is visible
        if (blinker_calibration_icon.is_visible()) {
            u8g2.setFont(u8g2_font_4x6_tr);
            u8g2.drawStr(46, 7, "CAL");
        }
        
        // Draw voltage display
        u8g2.setFont(u8g2_font_5x7_tr);
        uint16_t volt = voltage_controller.getVoltage();
        snprintf(print_buffer, sizeof(print_buffer), "%dV", volt);
        u8g2.drawStr(105, 42, print_buffer);
        
        // Highlight voltage box if target voltage is reached
        if (abs(volt - voltage_controller.getTargetVolt()) < 10) {
            u8g2.setDrawColor(2); // Inverse/toggle draw color
            u8g2.drawBox(104, 35, 21, 8);
        }
        u8g2.setDrawColor(1); // Restore draw color
        
        // Draw clock
        u8g2.setFont(u8g2_font_5x7_tr);
        snprintf(print_buffer, sizeof(print_buffer), "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
        u8g2.drawStr(97, anim_clock_y, print_buffer);
        
        // Draw widgets (icons, brace, histogram)
        icon_sounding.draw();
        icon_alarm.draw();
        icon_battery.draw();
        brace.draw();
        
        // If histogram is expanded (e.g., in stats view), draw stats overlay
        if (histogram.isExpanded()) {
            u8g2.clearBuffer(); // Clear screen for expanded view
            u8g2.drawStr(0, 10, "<STATS>");
            u8g2.drawStr(0, 20, "Max:");
            if (!use_cpm) {
                snprintf(print_buffer, sizeof(print_buffer), "%.3guSv/h", histogram.getMaxValue());
            } else {
                snprintf(print_buffer, sizeof(print_buffer), "%dCPM", (int)histogram.getMaxValue());
            }
            u8g2.drawStr(0, 30, print_buffer);
            
            u8g2.drawStr(0, 40, "Avg:");
            if (!use_cpm){
                snprintf(print_buffer, sizeof(print_buffer), "%.3guSv/h", histogram.getAverageValue());
            } else {
                snprintf(print_buffer, sizeof(print_buffer), "%dCPM", (int)histogram.getAverageValue());
            }
            u8g2.drawStr(0, 50, print_buffer);
        }
        
        histogram.draw();
        m_focusMan.draw();
    }
    
    // Handles user input events
    bool handleInput(InputEvent event) override {
        // Check if an active widget (e.g., expanded histogram) is consuming input
        IWidget* activeWidget = m_focusMan.getActiveWidget();
        if (activeWidget) {
            // Pass the event to the active widget
            if (activeWidget->handleEvent(event)) {
                // If the widget returns true, it signals that it has finished and control should return to FocusManager
                m_focusMan.clearActiveWidget();
            }
            return true; // Event handled
        }
        
        // Handle input for focus navigation
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
    
    // Called when the application is exited
    void onExit() {
        voltage_controller.stop();
        stop_counter_task();
        if (animationCoroutine_) {
            m_ui.removeCoroutine(animationCoroutine_);
            animationCoroutine_.reset();
        }

        if (alertSchedulerCoroutine_) {
            m_ui.removeCoroutine(alertSchedulerCoroutine_);
            alertSchedulerCoroutine_.reset();
        }
        m_ui.clearAllAnimations();
        m_ui.setContinousDraw(false);
        m_ui.markFading();
    }
};

// Application registry entry
AppItem counter_app{
    .title = "盖革计数器",
    .bitmap = image_counter_bits,
    
    // Factory function to create an instance of APP_COUNTER
    .createApp = [](PixelUI& ui) -> std::unique_ptr<IApplication> { 
        return std::make_unique<APP_COUNTER>(ui); 
    },
};