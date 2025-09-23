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
public:
    Penis(PixelUI& ui) : m_ui(ui) {}
    void draw() override {
        U8G2& display = m_ui.getU8G2();
        
        timestpNow = m_ui.getCurrentTime();

        if (timestpNow - timestpPrev > 50) {
            timestpPrev = timestpNow;
            if (state) {
                m_ui.animate(height, 35, 50, EasingType::LINEAR);
            } else {
                m_ui.animate(height, 28, 50, EasingType::LINEAR);
            }
            state = !state;
        }
        
        display.setFontMode(1);
        display.setBitmapMode(1);
        display.drawEllipse(69, 30, 4, 4);
        display.drawEllipse(54, 30, 4, 4);
        display.drawRFrame(58, 27, 8, height, 3);
    }

    bool handleInput(InputEvent event) override {
        if (event == InputEvent::BACK) {
            requestExit(); 
            return true;
        }
        return false;
    }
    
    void onEnter(ExitCallback cb) override {
        IApplication::onEnter(cb);
        m_ui.setContinousDraw(true);
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
    
    .type = MenuItemType::App,
    .order = 0
};