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

#include <memory>
#include <cmath>

#include "esp_app_desc.h"
#include "core/app/IApplication.h"
#include "core/app/app_system.h"
#include "core/coroutine/Coroutine.h"
#include "widgets/label/label.h"
#include "blinker/Blinker.h"

__attribute__((aligned(4)))
static const unsigned char image_about_bits[] U8X8_PROGMEM = {0xf0,0xff,0x0f,0xfc,0xff,0x3f,0xfe,0xff,0x7f,0xfe,0xff,0x77,0xff,0xc3,0xef,0xff,0x81,0xff,0xff,0x1c,0xff,0xff,0x3e,0xff,0xff,0x3f,0xff,0xff,0x3f,0xff,0xff,0x1f,0xff,0xff,0x9f,0xff,0xff,0x8f,0xff,0xff,0xc7,0xff,0xff,0xe7,0xff,0xff,0xe7,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xef,0xe7,0xff,0xdf,0xe7,0xff,0xfe,0xff,0x7f,0xfe,0xff,0x7f,0xfc,0xff,0x3f,0xf0,0xff,0x0f};

class About : public IApplication {
private:
    PixelUI& m_ui;
    Blinker bl;
    int32_t anim_h_line = 44;
    int32_t anim_phrase_1_y = 75;
    int32_t anim_phrase_2_y = 75;
    int32_t anim_phrase_3_y = 75;
    int32_t anim_phrase_4_y = 75;
    bool once_flag = false;

    std::shared_ptr<Coroutine> animationCoroutine_;

    void animation_coroutine_body(CoroutineContext& ctx, PixelUI& ui) {
        CORO_BEGIN(ctx);
        CORO_DELAY(ctx, ui, 160, 100);
        m_ui.animate(anim_phrase_1_y, 51, 300, EasingType::EASE_OUT_QUAD);
        CORO_DELAY(ctx, ui, 200, 200);
        m_ui.animate(anim_phrase_2_y, 60, 300, EasingType::EASE_OUT_QUAD);
        CORO_DELAY(ctx, ui, 200, 300);
        m_ui.animate(anim_phrase_3_y, 52, 300, EasingType::EASE_OUT_QUAD);
        CORO_DELAY(ctx, ui, 200, 400);
        m_ui.animate(anim_phrase_4_y, 61, 300, EasingType::EASE_OUT_QUAD);
        CORO_END(ctx);
    }

public:
    About(PixelUI& ui):m_ui(ui), bl(ui) {};
    void draw() override {

        if (!once_flag) {
            once_flag = true;
            m_ui.animate(anim_h_line, 61, 500, EasingType::EASE_OUT_CUBIC);
        }
        m_ui.markDirty();
        U8G2& u8g2 = m_ui.getU8G2();

        bl.update();

        u8g2.setFont(u8g2_font_5x7_tr);
        
        u8g2.drawStr(5, anim_phrase_1_y, "DESIGNED BY");
        u8g2.drawStr(5, anim_phrase_2_y, "LINKATOMS");

        u8g2.setFont(u8g2_font_6x10_tr);
        
        char buf[20] = "fw: ";

        if (bl.is_visible()) {u8g2.drawStr(12, 14, "THANKS FOR UR WORK");}
        else { 
            strcat(buf, esp_app_get_description()->version);
            u8g2.drawStr(12, 14, buf);
        }
        u8g2.drawStr(95, anim_phrase_3_y, "UNDER");

        u8g2.drawStr(84, anim_phrase_4_y, "PixelUI");

        u8g2.drawFrame(6, 1, 117, 18);

        u8g2.drawFrame(8, 3, 117, 18);

        u8g2.drawLine(63, 44, 63, anim_h_line);

        u8g2.drawStr(6, 36, "GENERAL");
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
        m_ui.markDirty(); 
        bl.set_interval(2000);
        bl.start();
        animationCoroutine_ = std::make_shared<Coroutine>( // add coroutine for the *Amazing* setup animation
            std::bind(&About::animation_coroutine_body, this, std::placeholders::_1, std::placeholders::_2),
            m_ui
        );

        m_ui.addCoroutine(animationCoroutine_);
    }

    void onExit() {
        if (animationCoroutine_) {
            m_ui.removeCoroutine(animationCoroutine_);
            animationCoroutine_.reset();
        }
        m_ui.setContinousDraw(false);
        m_ui.markFading();
    }
};

AppItem about_app{
    .title = "关于",
    .bitmap = image_about_bits,
    
    .createApp = [](PixelUI& ui) -> std::unique_ptr<IApplication> { 
        return std::make_unique<About>(ui); 
    },
};