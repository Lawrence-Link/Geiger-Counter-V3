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

#include "focus/focus.h"
#include "widgets/num_scroll/num_scroll.h"
#include "widgets/text_button/text_button.h"

__attribute__((aligned(4)))
static const unsigned char image_games_bits[] = {
    0xf0,0xff,0x0f,0x1c,0x00,0x38,0xde,0xff,0x77,0xde,0xff,0x77,0x5f,0x00,0xf4,0x5f,0x53,0xf5,0x5f,0x77,0xf5,0x5f,0xff,0xf5,0x5f,0xfd,0xf5,0x5f,0xec,0xf5,0x5f,0x84,0xf4,0x5f,0x00,0xf6,0xdf,0xff,0xf7,0xdf,0xd7,0xf7,0xdf,0xfe,0xf4,0x5f,0x9c,0xf4,0xdf,0x9e,0xf7,0xdf,0xff,0xf7,0xdf,0xd7,0xf7,0xdf,0xeb,0xf7,0xde,0xff,0x7b,0x1e,0x00,0x7c,0xfc,0xff,0x3f,0xf0,0xff,0x0f};

// user defined "Application"
// this is a great example of the animation drawing capability provided by the template.

class AppGames : public IApplication {
private:
    PixelUI& m_ui;
    FocusManager focusMan;

    uint32_t timestamp_1s_prev;
    uint32_t timestamp_1s_now;
public:
    AppGames(PixelUI& ui) : m_ui(ui), 
    focusMan(ui){}

    void onEnter(ExitCallback cb) override {
        IApplication::onEnter(cb);
        m_ui.setContinousDraw(true);
        timestamp_1s_now = timestamp_1s_prev = m_ui.getCurrentTime();
    }

    void draw() override {
        U8G2& display = m_ui.getU8G2();
        timestamp_1s_now = m_ui.getCurrentTime();
        if (timestamp_1s_now - timestamp_1s_prev > 1000) {
            timestamp_1s_prev = timestamp_1s_now;
            
        }
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
        } 
        else if (event == InputEvent::RIGHT) {
            focusMan.moveNext();
        } 
        else if (event == InputEvent::LEFT) {
            focusMan.movePrev();
        } 
        else if (event == InputEvent::SELECT) {
            focusMan.selectCurrent();
        }
        return true;
    }
    
    

    void onExit() override {
        m_ui.setContinousDraw(false);
    }
};

// self registering machanism, using a static AppRegistrar object to auto-register this app before main()

AppItem app_games{
    .title = "游戏",
    .bitmap = image_games_bits,
    
    .createApp = [](PixelUI& ui) -> std::unique_ptr<IApplication> { 
        return std::make_unique<AppGames>(ui); 
    },
};