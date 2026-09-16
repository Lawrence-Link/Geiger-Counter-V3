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
#include "PixelUI.h"
#include "core/app/app_system.h"
#include "system_nvs_varibles.h"
#include "widgets/label/label.h"
#include <memory>
#include <cmath>

extern AppItem first_boot_app;

class App_Boot : public IApplication {
private:
    PixelUI& m_ui;
    Label product_name;
    bool exit_flag = false;
    bool launch_first_boot = false;
    Coroutine coroutine_anim;
    int32_t line_width = 0;

    void startAnimation() {
        exit_flag = false;
        line_width = 0;
        coroutine_anim.reset();
        coroutine_anim.start();
        m_ui.addCoroutine(&coroutine_anim);
        m_ui.setContinousDraw(true);
        m_ui.markDirty();
    }

public:
    App_Boot(PixelUI& ui, void* param) : m_ui(ui), product_name(ui, 16, 36, "Geiger Counter N1", POS::BOTTOM),
    coroutine_anim([this](CoroutineContext& ctx) {
        CORO_BEGIN(ctx);
        CORO_DELAY(ctx, m_ui, 160, 100);
        product_name.onLoad();
        m_ui.animate(line_width, 97, 1000, EasingType::EASE_OUT_CUBIC, PROTECTION::PROTECTED);
        CORO_DELAY(ctx, m_ui, 1000, 200);
        exit_flag = true;
        CORO_END(ctx);
    }) {};

    void draw() override {
        m_ui.markDirty();
        if (launch_first_boot) {
            ViewManager* view_manager = m_ui.getViewManagerPtr();
            if (!view_manager->isTransitioning() &&
                view_manager->launch(first_boot_app) ==
                    ViewManager::LaunchResult::Ok) {
                launch_first_boot = false;
            }
            return;
        }

        U8G2& u8g2 = m_ui.getU8G2();
        product_name.draw();
        u8g2.drawHLine(14, 40, line_width);
        u8g2.setFont(u8g2_font_wqy12_t_gb2312);
        // u8g2.drawUTF8(1, 63, "test");
        if (exit_flag) { requestExit(); }   
    }

    bool handleInput(InputEvent event) override {
        if (launch_first_boot) return true;
        requestExit();
        return true;
    }
    
    void onEnter(ExitCallback cb) override {
        IApplication::onEnter(cb);
        if (SystemConf::getInstance().read_conf_first_boot()) {
            launch_first_boot = true;
            m_ui.setContinousDraw(true);
            m_ui.markDirty();
            return;
        }
        startAnimation();
    }

    void onPause() override {
        m_ui.setContinousDraw(false);
    }

    void onResume() override {
        launch_first_boot = false;
        startAnimation();
    }

    void onExit() {
        m_ui.setContinousDraw(false);
        m_ui.clearAllAnimations();
        m_ui.markFading();
        m_ui.removeCoroutine(&coroutine_anim);
    }
};

AppItem boot_app{
    .title = nullptr,
    .bitmap = nullptr,
    .factory = ApplicationFactory::make<App_Boot>(),
};
