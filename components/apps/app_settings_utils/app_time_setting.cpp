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
#include "core/coroutine/Coroutine.h"

#include "focus/focus.h"

#include "widgets/num_scroll/num_scroll.h"
#include "widgets/analog_clock/analog_clock.h"
#include "widgets/text_button/text_button.h"
#include "widgets/label/label.h"

#include "time_module.h"
#include "i2c.h"

class TimeSetting : public IApplication {
private:
    PixelUI& m_ui;
    NumScroll num_h, num_m, num_s;
    FocusManager m_focusman;
    Clock clock;
    TextButton button_sync;
    Label title;
    std::shared_ptr<Coroutine> animationCoroutine_;

    int32_t anim_title_bar = 0;
    int32_t anim_title_x = -50;
    int32_t anim_analog_clock_x = 103;

    struct tm timeinfo_adjust;
    struct tm timeinfo_realtime;
    bool tm_valid;

    void animation_load_coroutine(CoroutineContext& ctx, PixelUI& ui) {
    CORO_BEGIN(ctx);
        
    CORO_DELAY(ctx,ui, 100, 1); // wait for renderer loading
        clock.onLoad();
        num_h.onLoad();
    CORO_DELAY(ctx, ui, 100, 12);
        title.onLoad();
        num_m.onLoad();
        m_ui.animate(anim_title_bar, 78, 700, EasingType::EASE_IN_OUT_CUBIC, PROTECTION::PROTECTED);
        m_ui.animate(anim_title_x, 3, 300, EasingType::EASE_IN_OUT_CUBIC, PROTECTION::PROTECTED);
    CORO_DELAY(ctx, ui, 100, 123);
        num_s.onLoad();
    CORO_DELAY(ctx, ui, 100, 11);
        button_sync.onLoad();
    CORO_END(ctx);
    }

    uint32_t timestamp_prev;
    uint32_t timestamp_now;

    enum {ADJUSTING, SET} state = ADJUSTING;

public:
    TimeSetting(PixelUI& ui): m_ui(ui), 
    num_h(ui), 
    num_m(ui), 
    num_s(ui), 
    m_focusman(ui),
    clock(ui),
    button_sync(ui),
    title(ui, 3, 14, "RTC时间") {};

    void draw() override {
        U8G2& u8g2 = m_ui.getU8G2();
        
        timestamp_now = m_ui.getCurrentTime();
        if (timestamp_now - timestamp_prev > 1000) {
            timestamp_prev = timestamp_now;
            pcf8563_get_time(pcf8563_dev, &timeinfo_realtime, &tm_valid);
        }

        u8g2.drawHLine(0, 19, anim_title_bar);
        
        u8g2.setFont(u8g2_font_5x7_tr);
        char buf[10];
        snprintf(buf, 10, "%02d:%02d:%02d", timeinfo_realtime.tm_hour, timeinfo_realtime.tm_min, timeinfo_realtime.tm_sec);
        u8g2.drawStr(84, 62, buf);

        switch (state) {        
            case ADJUSTING:
            clock.setHour(num_h.getValue());
            clock.setMinute(num_m.getValue());
            clock.setSecond(num_s.getValue());
            break;
            case SET:
            clock.setHour(timeinfo_realtime.tm_hour);
            clock.setMinute(timeinfo_realtime.tm_min);
            clock.setSecond(timeinfo_realtime.tm_sec);
            break;
        }

        num_h.draw();
        num_m.draw();
        num_s.draw();
        clock.draw();
        button_sync.draw();
        title.draw();
        
        m_focusman.draw();
    }

    bool handleInput(InputEvent event) override {
        IWidget* activeWidget = m_focusman.getActiveWidget();
        if (activeWidget) {
            // If so, pass the event to that widget
            if (activeWidget->handleEvent(event)) {
                // If the widget returns true, it means it has finished processing and control is handed back to the FocusManager
                m_focusman.clearActiveWidget();
            }
            return true; // Event has been handled, return true
        }
        // No widget has taken over input, execute the original focus management logic
        if (event == InputEvent::BACK) {
            requestExit();
        } else if (event == InputEvent::RIGHT) {
            m_focusman.moveNext();
        } else if (event == InputEvent::LEFT) {
            m_focusman.movePrev();
        } else if (event == InputEvent::SELECT) {
            m_focusman.selectCurrent();
        }
        return true;
    }
    
    void onEnter(ExitCallback cb) override {
        IApplication::onEnter(cb);
        m_ui.setContinousDraw(true);
        m_ui.markDirty(); 

        num_h.setPosition(1,25);
        num_h.setRange(0,23);
        num_h.setSize(24, 16);
        num_h.setValue(0);
        num_h.setFixedIntDigits(2);

        num_m.setPosition(27,25);
        num_m.setRange(0,59);
        num_m.setSize(24, 16);
        num_m.setValue(0);
        num_m.setFixedIntDigits(2);

        num_s.setPosition(53,25);
        num_s.setRange(0,59);
        num_s.setSize(24, 16);
        num_s.setValue(0);
        num_s.setFixedIntDigits(2);

        clock.setPosition(103, 32);
        clock.setRadius(20);

        pcf8563_get_time(pcf8563_dev, &timeinfo_adjust, &tm_valid);
        pcf8563_get_time(pcf8563_dev, &timeinfo_realtime, &tm_valid);

        num_h.setValue(timeinfo_realtime.tm_hour);
        num_m.setValue(timeinfo_realtime.tm_min);
        num_s.setValue(timeinfo_realtime.tm_sec);

        button_sync.setPosition(1, 44);
        button_sync.setSize(76, 17);
        button_sync.setText("写入");
        button_sync.setCallback([this](){
            if (state == ADJUSTING) {
                timeinfo_adjust.tm_hour = num_h.getValue();
                timeinfo_adjust.tm_min = num_m.getValue();
                timeinfo_adjust.tm_sec = num_s.getValue();
                if (pcf8563_set_time(pcf8563_dev, &timeinfo_adjust) == ESP_OK) {
                    button_sync.setText("成功");
                    state = SET;
                }
            } else {
                requestExit();
            }
            
        });

        animationCoroutine_ = std::make_shared<Coroutine>(
            std::bind(&TimeSetting::animation_load_coroutine, this, std::placeholders::_1, std::placeholders::_2), m_ui
        );

        m_ui.addCoroutine(animationCoroutine_);

        m_focusman.addWidget(&num_h);
        m_focusman.addWidget(&num_m);
        m_focusman.addWidget(&num_s);
        m_focusman.addWidget(&button_sync);

        timestamp_prev = timestamp_now = m_ui.getCurrentTime();
    }

    void onResume() override {
        m_ui.setContinousDraw(true);
    }

    void onExit() override {
        // cleanup the coroutine
        if (animationCoroutine_) {
            m_ui.removeCoroutine(animationCoroutine_);
            animationCoroutine_.reset();
        }
        m_ui.clearAllAnimations();
        m_ui.setContinousDraw(false);
        m_ui.markFading();
    }
};

AppItem time_setting_app{
    .title = "TimeSetting Demo",
    .bitmap = nullptr,
    
    .createApp = [](PixelUI& ui) -> std::unique_ptr<IApplication> { 
        return std::make_unique<TimeSetting>(ui); 
    },
};