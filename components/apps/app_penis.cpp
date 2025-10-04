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
#include <memory>
#include <iostream>
#include "focus/focus.h"
#include "esp_log.h"
#include "widgets/num_scroll/num_scroll.h"
#include "widgets/text_button/text_button.h"

static const unsigned char image_sans4_bits[] = {
    0xf0,0xff,0x0f,0xfc,0xfb,0x3f,0xfe,0xf7,0x7f,0xfe,0xeb,0x7f,0xff,0xf7,0xff,0xff,0xf7,0xff,0xff,0xf7,0xff,0xff,0xe3,0xff,0xff,0xdd,0xff,0xff,0xdd,0xff,0xff,0xdd,0xff,0xff,0xdd,0xff,0xff,0xdd,0xff,0xff,0xdd,0xff,0xff,0xdd,0xff,0xff,0xdd,0xff,0xff,0xdd,0xff,0x3f,0x1c,0xfe,0xbf,0xdd,0xfe,0xbf,0xdd,0xfe,0x7e,0x3e,0x7f,0xfe,0xff,0x7f,0xfc,0xff,0x3f,0xf0,0xff,0x0f};

static int timestpPrev = 0;
int timestpNow;
int32_t height = 28;
bool state = 0;

// user defined "Application"
// this is a great example of the animation drawing capability provided by the template.

class Penis : public IApplication {
private:
    PixelUI& m_ui;
    FocusManager focusMan;
    NumScroll num,  num2;
    TextButton btn1;
public:
    Penis(PixelUI& ui) : m_ui(ui), 
    focusMan(ui), num(ui), num2(ui), btn1(ui) {}
    void draw() override {
        U8G2& display = m_ui.getU8G2();
        
        timestpNow = m_ui.getCurrentTime();

        if (timestpNow - timestpPrev > 50) {
            timestpPrev = timestpNow;
            if (state) {
                m_ui.animate(height, 35, 50, EasingType::LINEAR, PROTECTION::PROTECTED);
            } else {
                m_ui.animate(height, 28, 50, EasingType::LINEAR, PROTECTION::PROTECTED);
            }
            state = !state;
        }
        
        display.setFontMode(1);
        display.setBitmapMode(1);
        display.drawEllipse(69, 30, 4, 4);
        display.drawEllipse(54, 30, 4, 4);
        display.drawRFrame(58, 27, 8, height, 3);

        num.draw();
        num2.draw();
        btn1.draw();

        focusMan.draw();
    }

    bool handleInput(InputEvent event) override {
        // Check if an active widget (e.g., expanded histogram) is consuming input
        IWidget* activeWidget = focusMan.getActiveWidget();
        if (activeWidget) {
            // Pass the event to the active widget
            if (activeWidget->handleEvent(event)) {
                // If the widget returns true, it signals that it has finished and control should return to FocusManager
                focusMan.clearActiveWidget();
            }
            return true; // Event handled
        }
        
        // Handle input for focus navigation
        if (event == InputEvent::BACK) {
            requestExit();
        } else if (event == InputEvent::RIGHT) {
            focusMan.moveNext();
        } else if (event == InputEvent::LEFT) {
            focusMan.movePrev();
        } else if (event == InputEvent::SELECT) {
            focusMan.selectCurrent();
        }
        return true;
    }
    
    void onEnter(ExitCallback cb) override {
        IApplication::onEnter(cb);
        m_ui.setContinousDraw(true);
        num.setPosition(10,10);
        num.setRange(0,23);
        num.setSize(14, 16);
        num.setValue(0);
        num.setFixedIntDigits(1);

        num2.setPosition(37,10);
        num2.setRange(0,59);
        num2.setSize(24, 16);
        num2.setValue(0);
        num2.setFixedIntDigits(2);

        btn1.setCallback([](){ESP_LOGI("penis", "HI");});
        btn1.setText("Penis");
        btn1.setPosition(67, 10);
        btn1.setSize(34, 17);

        focusMan.addWidget(&num);
        focusMan.addWidget(&num2);
        focusMan.addWidget(&btn1);

        num.onLoad();
        num2.onLoad();
        btn1.onLoad();
    }

    void onExit() override {
        m_ui.setContinousDraw(false);
    }
};

// self registering machanism, using a static AppRegistrar object to auto-register this app before main()

AppItem app_penis{
    .title = "PENIS",
    .bitmap = image_sans4_bits,
    
    .createApp = [](PixelUI& ui) -> std::unique_ptr<IApplication> { 
        return std::make_unique<Penis>(ui); 
    },
};