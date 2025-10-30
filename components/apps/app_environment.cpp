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
#include "widgets/curve_chart/curve_chart.h"
#include "bme280_port.h"
#include "esp_log.h"

__attribute__((aligned(4)))
static const unsigned char image_environment_bits[] = {0xf0,0xff,0x0f,0xfc,0xff,0x3f,0xfe,0xff,0x7f,0xce,0xe7,0x7f,0xb7,0xdb,0xff,0xb7,0xbd,0xff,0x7b,0xb5,0xff,0xfd,0xa4,0xff,0xfd,0x34,0xf7,0xfe,0x25,0xe9,0xfe,0xb5,0xee,0xff,0xa5,0xdf,0xff,0xb5,0xdf,0xff,0x76,0xbf,0x7f,0xe7,0x7d,0x7f,0xdb,0xfa,0x7f,0x5b,0xf7,0x7f,0xa7,0xef,0xff,0xbe,0xee,0xff,0xbd,0xed,0xfe,0x43,0x77,0xfe,0xff,0x78,0xfc,0xff,0x3f,0xf0,0xff,0x0f};
__attribute__((aligned(4)))
static const uint8_t image_barometer_bits[] = {0xf0,0x01,0x08,0x02,0x04,0x04,0x02,0x09,0x81,0x10,0x81,0x10,0x41,0x10,0x41,0x10,0x21,0x10,0x21,0x10,0x30,0x00,0x10,0x00};
__attribute__((aligned(4)))
static const uint8_t image_humidity_bits[] = {0x20,0x00,0x20,0x00,0x30,0x00,0x70,0x00,0x78,0x00,0xf8,0x00,0xfc,0x01,0xfc,0x01,0x7e,0x03,0xfe,0x02,0xff,0x06,0xff,0x07,0xfe,0x03,0xfe,0x03,0xfc,0x01,0xf8,0x00};
__attribute__((aligned(4)))
static const uint8_t image_temperature_bits[] = {0x38,0x00,0x44,0x40,0xd4,0xa0,0x54,0x40,0xd4,0x1c,0x54,0x06,0xd4,0x02,0x54,0x02,0x54,0x06,0x92,0x1c,0x39,0x01,0x75,0x01,0x7d,0x01,0x39,0x01,0x82,0x00,0x7c,0x00};

class AppEnvironment : public IApplication {
private:
    PixelUI& m_ui;
    FocusManager focusMan;
    CurveChart chart_temp;
    CurveChart chart_humi;
    CurveChart chart_baro;

    uint32_t timestamp_1s_prev;
    uint32_t timestamp_1s_now;
    int32_t anim_img_temperature_x = -45;
    int32_t anim_img_humidity_x = -45;
    int32_t anim_img_barometer_x = -45;

    Coroutine coroutine_anim;

    char print_buffer[15];

public:
    AppEnvironment(PixelUI& ui) : 
    m_ui(ui), 
    focusMan(m_ui), 
    chart_temp(m_ui, 71, 2, 56, 19), 
    chart_humi(m_ui, 71, 23, 56, 19), 
    chart_baro(m_ui, 71, 44, 56, 19),
    coroutine_anim([this](CoroutineContext& ctx) 
    {
        CORO_BEGIN(ctx);
        CORO_DELAY(ctx, m_ui, 50, 1);
        chart_temp.onLoad(); 
        m_ui.animate(anim_img_temperature_x, 0, 320, EasingType::EASE_OUT_QUAD, PROTECTION::PROTECTED);
        CORO_DELAY(ctx, m_ui, 50, 2);
        chart_humi.onLoad(); 
        m_ui.animate(anim_img_humidity_x, 0, 320, EasingType::EASE_OUT_QUAD, PROTECTION::PROTECTED);
        CORO_DELAY(ctx, m_ui, 50, 3);
        chart_baro.onLoad();
        m_ui.animate(anim_img_barometer_x, 0, 320, EasingType::EASE_OUT_QUAD, PROTECTION::PROTECTED);
        CORO_END(ctx);
    })
    {

    }

    void onEnter(ExitCallback cb) override {
        IApplication::onEnter(cb);
        m_ui.setContinousDraw(true);
        timestamp_1s_now = timestamp_1s_prev = m_ui.getCurrentTime();

        chart_temp.setExpand(EXPAND_BASE::BOTTOM_RIGHT, 110, 19);
        chart_humi.setExpand(EXPAND_BASE::BOTTOM_RIGHT, 110, 19);
        chart_baro.setExpand(EXPAND_BASE::BOTTOM_RIGHT, 110, 19);

        focusMan.addWidget(&chart_temp);
        focusMan.addWidget(&chart_humi);
        focusMan.addWidget(&chart_baro);

        coroutine_anim.reset();
        coroutine_anim.start();

        m_ui.addCoroutine(&coroutine_anim);
    }

    void draw() override {
        U8G2& u8g2 = m_ui.getU8G2();
        timestamp_1s_now = m_ui.getCurrentTime();

        if (timestamp_1s_now - timestamp_1s_prev > 500) {
            timestamp_1s_prev = timestamp_1s_now;
            chart_temp.addData(bme280_read_temperature_celsius());
            chart_humi.addData(bme280_read_humidity_percentage());
            chart_baro.addData(bme280_read_barometer());
        }

        u8g2.setFont(u8g2_font_missingplanet_tr);
        snprintf(print_buffer, sizeof(print_buffer), "%.2f °C", bme280_read_temperature_celsius());
        u8g2.drawStr(19+anim_img_temperature_x, 15, print_buffer);
        snprintf(print_buffer, sizeof(print_buffer), "%.2f %%", bme280_read_humidity_percentage());
        u8g2.drawStr(19+anim_img_humidity_x, 39, print_buffer);
        snprintf(print_buffer, sizeof(print_buffer), "%.2fkPa", bme280_read_barometer() / 1000);
        u8g2.drawStr(19+anim_img_barometer_x, 61, print_buffer);

        u8g2.drawXBMP(anim_img_temperature_x, 2, 16, 16, image_temperature_bits);
        u8g2.drawXBMP(anim_img_humidity_x, 25, 11, 16, image_humidity_bits);
        u8g2.drawXBMP(anim_img_barometer_x, 49, 13, 12, image_barometer_bits);

        chart_baro.draw();
        chart_humi.draw();
        chart_temp.draw();
        focusMan.draw();
    }

    bool handleInput(InputEvent event) override {
        // Check if an active widget (e.g., expanded chartgram) is consuming input
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
    
    

    void onExit() override {
        m_ui.removeCoroutine(&coroutine_anim);
        m_ui.setContinousDraw(false);
        m_ui.markFading();
    }
};

// self registering machanism, using a static AppRegistrar object to auto-register this app before main()

AppItem app_environment{
    .title = "BME280传感器",
    .bitmap = image_environment_bits,
    
    .createApp = [](PixelUI& ui) -> std::unique_ptr<IApplication> { 
        return std::make_unique<AppEnvironment>(ui); 
    },
};