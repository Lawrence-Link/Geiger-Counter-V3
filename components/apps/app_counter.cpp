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
static const unsigned char image_info_bits[] = {
    0xf0,0xff,0x0f,0xfc,0xff,0x3f,0xfe,0xff,0x7f,0xfe,0xff,0x7f,0xff,0x81,0xff,0xff,0x00,0xff,0x7f,0x3e,0xff,0x7f,0x3f,0xff,0xff,0x3f,0xff,0xff,0x1f,0xff,0xff,0x8f,0xff,0xff,0xc7,0xff,0xff,0xe3,0xff,0xff,0xe3,0xff,0xff,0xe3,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xe3,0xff,0xff,0xe3,0xff,0xff,0xff,0xff,0xfe,0xff,0x7f,0xfe,0xff,0x7f,0xfc,0xff,0x3f,0xf0,0xff,0x0f
};
static const unsigned char image_Background_bits[] = {0xfe,0x01,0x00,0x00,0x00,0x00,0x00,0xe0,0xff,0xff,0xff,0x0f,0x00,0x00,0x00,0x00,0x01,0x03,0x00,0x00,0x00,0x00,0x00,0x30,0x00,0x00,0x00,0x18,0x00,0x00,0x00,0x00,0x7d,0x06,0x00,0x00,0x00,0x00,0x00,0x18,0xff,0xb7,0x55,0x31,0x00,0x00,0x00,0x00,0x81,0xfc,0xff,0xff,0xff,0xff,0xff,0x8f,0x00,0x00,0x00,0xe2,0xff,0xff,0xff,0x7f,0x3d,0x01,0x00,0x00,0x00,0x00,0x00,0x40,0xb6,0xea,0xff,0x04,0x00,0x00,0x00,0x80,0x41,0xfe,0xff,0xff,0xaa,0xfe,0xff,0x3f,0x01,0x00,0x00,0xf9,0xff,0xff,0xff,0xab,0x9f,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0xf8,0xff,0x7f,0x02,0x00,0x00,0x00,0x80,0x20,0xff,0xff,0xff,0xff,0x55,0xfd,0x7f,0xfc,0xff,0xff,0x6c,0xff,0xff,0xff,0xb5,0x40,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x06,0x00,0x80,0x01,0x00,0x00,0x00,0x80,0x80,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x03,0x00,0x00,0xff,0xff,0xff,0xff,0xff};
static float s_static_data_buffer[25] = {
    0.1f, 0.2f, 0.4f, 0.6f, 0.8f, 1.0f, 0.9f, 0.7f, 0.5f, 0.3f,
    0.2f, 0.1f, 0.3f, 0.5f, 0.7f, 0.9f, 1.0f, 0.8f, 0.6f, 0.4f,
    0.2f, 0.1f, 0.2f, 0.3f, 0.4f
};
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
// --- USER DEFINED APP: A Geiger counter UI demo ---
class APP_COUNTER: public IApplication {
private:
    PixelUI& m_ui;
    Histogram histogram;
    Brace brace;
    FocusManager m_focusMan;
    IconButton icon_battery;
    IconButton icon_alert;
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
public:
    APP_COUNTER(PixelUI& ui) : 
    m_ui(ui), 
    histogram(ui), 
    brace(ui), 
    m_focusMan(ui), 
    icon_battery(ui),
    icon_alert(ui),
    icon_sounding(ui),
    icon_alarm(ui)
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
        icon_battery.setCoordinate(14, 2);
        // icon sounding
        icon_sounding.setSource(image_SOUND_OFF_bits);
        icon_sounding.setMargin(7, 7);
        icon_sounding.setCoordinate(40, 1);
        // icon alert
        icon_alert.setSource(image_Alert_bits);
        icon_alert.setMargin(9, 7);
        icon_alert.setCoordinate(28, 1);
        // icon alarm
        icon_alarm.setSource(image_BELL_bits);
        icon_alarm.setMargin(6, 7);
        icon_alarm.setCoordinate(51, 1);
        // Adding widgets to focus manager, enabling cursor navigation
        m_focusMan.addWidget(&brace);
        m_focusMan.addWidget(&histogram);  
        loadState = LoadState::INIT;
        first_time = false;
    }
    void braceContent() {
        U8G2& u8g2 = m_ui.getU8G2();
        u8g2.setFont(u8g2_font_5x7_tr);
        u8g2.drawStr(30, 54, "10.00");
        u8g2.drawStr(31, 61, "uSv/h");
        u8g2.drawRBox(8, 50, 20, 10, 2);
        u8g2.setDrawColor(0);
        u8g2.drawStr(11, 58, "Max");
        u8g2.setDrawColor(1);
    }
    void draw() override {
        if (!first_time) {
            m_ui.animate(anim_mark_m, 23, 300, EasingType::EASE_OUT_QUAD, PROTECTION::PROTECTED);
            m_ui.animate(anim_bg, 128, 500, EasingType::EASE_IN_OUT_CUBIC, PROTECTION::PROTECTED);
            
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
                icon_alert.onLoad();
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
        
        u8g2.setFont(u8g2_font_5x7_tr);
        u8g2.drawStr(5, 42, "MEAS");
        u8g2.setClipWindow(29,36,83,42);
        u8g2.drawStr(anim_status_x, 42, "PLEASE WAIT");
        u8g2.setMaxClipWindow();
        u8g2.setDrawColor(2);
        u8g2.drawBox(3, 35, anim_mark_m, 8);
        u8g2.setDrawColor(1);
        u8g2.setFont(u8g2_font_profont17_tr);
        u8g2.drawStr(3, 31, "-.-- mR/h");
        u8g2.setFont(u8g2_font_4x6_tr);
        u8g2.drawStr(100, 32, "CNT");
        u8g2.drawStr(100, 39, "1234");
        histogram.setData(s_static_data_buffer, 25, 0);
        
        // draw widgets
        icon_sounding.draw();
        icon_alarm.draw();
        icon_alert.draw();
        icon_battery.draw();
        brace.draw();
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