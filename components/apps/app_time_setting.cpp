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
#include "widgets/num_scroll/num_scroll.h"

class TimeSetting : public IApplication {
private:
    PixelUI& m_ui;
    NumScroll num_h, num_m, num_s;
public:
    TimeSetting(PixelUI& ui):m_ui(ui) {};
    void draw() override {
        U8G2& u8g2 = m_ui.getU8G2();
    }

    bool handleInput(InputEvent event) override {
        return false;
    }
    
    void onEnter(ExitCallback cb) override {
        IApplication::onEnter(cb);
        m_ui.setContinousDraw(true);
        m_ui.markDirty(); 
    }

    void onResume() override {
        m_ui.setContinousDraw(true);
    }

    void onExit() override {
        m_ui.setContinousDraw(false);
        m_ui.markFading();
    }
};

AppItem time_setting_app{
    .title = nullptr,
    .bitmap = nullptr,
    
    .createApp = [](PixelUI& ui) -> std::unique_ptr<IApplication> { 
        return std::make_unique<TimeSetting>(ui); 
    },
};
