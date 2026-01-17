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
#include "blinker/Blinker.h"
#include "voltage_pid.hpp"
#include "tune.h"
#include "system_nvs_varibles.h"
#include <memory>

static const unsigned char image_ArrowUpFilled_bits[] U8X8_PROGMEM = {0xc0,0x00,0x20,0x01,0xd0,0x02,0xe8,0x05,0xf4,0x0b,0xfa,0x17,0x61,0x21,0xaf,0x3d,0x68,0x05,0xa8,0x05,0x68,0x05,0xa8,0x05,0xe8,0x05,0x08,0x04,0xf8,0x07};
static const unsigned char image_back_btn_bits[] U8X8_PROGMEM = {0x04,0x00,0x06,0x00,0xff,0x00,0x06,0x01,0x04,0x02,0x00,0x02,0x00,0x01,0xf8,0x00};
static const unsigned char image_device_reset_bits[] U8X8_PROGMEM = {0x00,0x00,0x00,0x00,0x00,0x00,0x98,0x07,0x84,0x03,0x82,0x03,0x81,0x04,0x01,0x04,0x01,0x04,0x01,0x04,0x01,0x04,0x02,0x02,0x04,0x01,0xf8,0x00,0x00,0x00,0x00,0x00};
static const unsigned char image_EQ_symbol_bits[] U8X8_PROGMEM = {0x0f,0x00,0x0f};
static const unsigned char image_Quest_bits[] U8X8_PROGMEM = {0x1e,0x33,0x33,0x30,0x18,0x0c,0x00,0x0c};
static const unsigned char image_Voltage_bits[] U8X8_PROGMEM = {0x00,0x0c,0x00,0x0c,0x00,0x0f,0x00,0x0f,0xc0,0x03,0xc0,0x03,0xf0,0x03,0xf0,0x03,0xfc,0x00,0xfc,0x00,0xff,0x3f,0xff,0x3f,0xc0,0x0f,0xc0,0x0f,0xf0,0x03,0xf0,0x03,0xf0,0x00,0xf0,0x00,0x3c,0x00,0x3c,0x00,0x0c,0x00,0x0c,0x00};
static const unsigned char image_ArrowDownFilled_bits[] U8X8_PROGMEM = {0xf8,0x07,0x08,0x04,0xe8,0x05,0x68,0x05,0xa8,0x05,0x68,0x05,0xa8,0x05,0x6f,0x3d,0xa1,0x21,0xfa,0x17,0xf4,0x0b,0xe8,0x05,0xd0,0x02,0x20,0x01,0xc0,0x00};

class PowerSurgeNotify : public IApplication {
private:
    PixelUI& m_ui;
    VoltagePID::vpid_error_t vpid_error;
    Blinker blinker;

    Tune::Melody tune_err = {
        {Notes::REST, 100},
        {Notes::B4, 100},
        {Notes::REST, 100},
        {Notes::C4, 100},
    };

public:
    PowerSurgeNotify(PixelUI& ui, void* param):m_ui(ui), blinker(ui) {
        vpid_error = static_cast<VoltagePID::vpid_error_t>(param ? *((int*)param) : VoltagePID::VPID_ERR_SURGE);
        blinker.set_interval(200);
        blinker.start();
    };
    void draw() override {
        blinker.update();
        U8G2& u8g2 = m_ui.getU8G2();

        u8g2.drawXBMP(9, 2, 14, 22, image_Voltage_bits);

        if (vpid_error == VoltagePID::vpid_error_t::VPID_ERR_SURGE) {
            u8g2.setFont(u8g2_font_6x12_tr);
            
            if (blinker.is_visible()) 
            {
                u8g2.drawStr(21, 61, "POWER CYCLE NOW");
                u8g2.drawXBMP(114, 49, 11, 16, image_device_reset_bits);
            }
            u8g2.drawXBMP(15, 56, 4, 3, image_EQ_symbol_bits);
            u8g2.drawXBMP(3, 53, 10, 8, image_back_btn_bits); 
            u8g2.drawStr(34, 10, "POWER SURGE");
            u8g2.drawStr(42, 23, "DETECTED");
            u8g2.drawLine(3, 28, 124, 28);
            
            u8g2.drawXBMP(106, 6, 14, 15, image_ArrowUpFilled_bits);
            u8g2.drawStr(21, 39, "ADJUST PIDS OR");
            u8g2.drawXBMP(8, 36, 7, 8, image_Quest_bits);
            u8g2.drawStr(21, 48, "RM SHORT CIRCUIT");
        } else if (vpid_error == VoltagePID::vpid_error_t::VPID_ERR_LACK_PWR) {
            u8g2.setFont(u8g2_font_6x12_tr);
            if (blinker.is_visible()) 
            {
                u8g2.drawStr(21, 61, "POWER CYCLE NOW");
                u8g2.drawXBMP(114, 49, 11, 16, image_device_reset_bits);
            }
            u8g2.drawXBMP(15, 56, 4, 3, image_EQ_symbol_bits);
            u8g2.drawXBMP(3, 53, 10, 8, image_back_btn_bits);

            u8g2.drawLine(3, 26, 124, 26);
            u8g2.setFont(u8g2_font_5x8_tr);
            u8g2.drawStr(18, 38, "TUBE SHORTED? OR TRY");
            u8g2.drawXBMP(5, 35, 7, 8, image_Quest_bits);
            u8g2.drawXBMP(109, 5, 14, 15, image_ArrowDownFilled_bits);
            u8g2.setFont(u8g2_font_6x12_tr);
            u8g2.drawStr(48, 13, "OUTPUT");
            u8g2.drawStr(37, 22, "RESTRICTED");
            u8g2.setFont(u8g2_font_5x8_tr);
            u8g2.drawStr(18, 46, "TUNE LOOP IMPEDANCE");
        }
    }
    bool handleInput(InputEvent event) override {
        if (event == InputEvent::SELECT || event == InputEvent::BACK) {
            esp_restart();
        }
        return false;
    }
    
    void onEnter(ExitCallback cb) override {
        IApplication::onEnter(cb);
        m_ui.setContinousDraw(true);
        m_ui.markDirty();
        if (SystemConf::getInstance().read_conf_enable_interaction_tone()) {
            Tune& tune = Tune::getInstance();
            tune.stop();
            tune.playMelody(tune_err);
        }
    }

    void onResume() override {
        // m_ui.markDirty();
        m_ui.setContinousDraw(true);
    }

    void onExit() override {
        m_ui.setContinousDraw(false);
        m_ui.markFading();
    }
};

AppItem power_surge_app
{
    .title = nullptr,
    .bitmap = nullptr,
    
    .createApp = [](PixelUI& ui, void* param) -> std::unique_ptr<IApplication> { 
        return std::make_unique<PowerSurgeNotify>(ui, param); 
    },
};