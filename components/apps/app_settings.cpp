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

// --- USER DEFINED APP: A ListView Demo ---

#include "PixelUI.h"
#include "core/app/app_system.h"
#include "ui/ListView/ListView.h"
#include "system_nvs_varibles.h"

#include "esp_log.h"

bool en_sound_click = false; // geiger click
bool en_sound_navigate = false; // sound navigate
bool en_sos = false;    // alert
bool en_led = true;     // led
float convertion_coefficient = 0.0f;
int32_t brightness = 0;
int32_t cpm_warn_threshold = 0;
int32_t cpm_dngr_threshold = 0;
int32_t cpm_hzdr_threshold = 0;
int32_t operation_voltage = 380;

static const unsigned char image_settings_bits[] = {0xf0,0xff,0x0f,0xfc,0xff,0x3f,0xfe,0xff,0x7f,0xfe,0xe7,0x7f,0xff,0xe7,0xff,0x9f,0x81,0xf9,0x1f,0x3c,0xf8,0x3f,0xff,0xfc,0xbf,0xc3,0xfd,0x9f,0x3d,0xf9,0xdf,0xfe,0xff,0xc7,0x7e,0xe8,0xc7,0xbe,0xeb,0xdf,0x7e,0xfb,0x9f,0xed,0xf6,0xbf,0xd5,0xf6,0x3f,0x37,0xf7,0x1f,0xf4,0xef,0x9f,0xc5,0xdf,0xff,0x3f,0xbe,0xfe,0xe7,0x7d,0xfe,0xff,0x7b,0xfc,0xff,0x37,0xf0,0xff,0x0f};

extern PixelUI ui;

auto& syscfg = SystemConf::getInstance();

ListItem sub_Alarm[5] = {
    ListItem(">>> 剂量警告 <<<"),
    ListItem("- 启用", nullptr, 0, nullptr, ListItemExtra{&en_sos, nullptr}),
    ListItem("- 警告阈值(CPM)", nullptr, 0, [](){  }, ListItemExtra{nullptr, &cpm_warn_threshold}),
    ListItem("- 危险阈值(CPM)", nullptr, 0, [](){  }, ListItemExtra{nullptr, &cpm_dngr_threshold}),
    ListItem("- 灾难阈值(CPM)", nullptr, 0, [](){  }, ListItemExtra{nullptr, &cpm_hzdr_threshold})
};

ListItem sub_cvs_cfg[4] = {
    ListItem(">>> 恒压PID调试 <<<"),
    ListItem("- Kp 比例", nullptr, 0, [](){  }),
    ListItem("- Ki 积分", nullptr),
    ListItem("- Kd 微分", nullptr)
};

ListItem sub_Tube_cfg[4] = {
    ListItem(">>> 盖革管 <<<"),
    ListItem("- 工作电压", nullptr, 0, [](){  }, ListItemExtra{nullptr, &operation_voltage}),
    ListItem("- 恒压调试", sub_cvs_cfg, 4),
    ListItem("- 转换系数")
};

ListItem itemList[7] = {
    ListItem(">>>> 设置 <<<<"),
    ListItem("- 屏幕亮度", nullptr, 0, [](){ ui.showPopupProgress(brightness, 0, 5, "亮度", 100, 40, 5000, 1, [](int32_t val){ui.getU8G2().setContrast(val * 51);}); }, ListItemExtra{nullptr, &brightness}),
    ListItem("- 剂量警告", sub_Alarm, 5),
    ListItem("- 盖革管", sub_Tube_cfg, 4),
    ListItem("- 计数音", nullptr, 0, nullptr, ListItemExtra{&en_sound_click, nullptr}),
    ListItem("- 导航音", nullptr, 0, [](){syscfg.set_conf_enable_navi_tone(en_sound_navigate);}, ListItemExtra{&en_sound_navigate, nullptr}),
    ListItem("- LED指示", nullptr, 0, nullptr, ListItemExtra{&en_led, nullptr})
};

class APP_SETTINGS : public ListView {
public:
    APP_SETTINGS(PixelUI& ui, ListItem *itemList, size_t length) : ListView(ui, itemList, length) {}
    
    void onLoad() override {
        auto& syscfg = SystemConf::getInstance();

        syscfg.load_conf_from_nvs();

        brightness = syscfg.read_conf_brightness();

        en_sound_click = syscfg.read_conf_enable_geiger_click();
        en_sos = syscfg.read_conf_enable_alert();
        en_sound_navigate = syscfg.read_conf_enable_navi_tone();
        en_led = syscfg.read_conf_enable_blink();
        convertion_coefficient = syscfg.read_conf_tube_convertion_coefficient();

        cpm_warn_threshold = syscfg.read_conf_warn_threshold();
        cpm_dngr_threshold = syscfg.read_conf_dngr_threshold();
        cpm_hzdr_threshold = syscfg.read_conf_hzdr_threshold();

        operation_voltage = syscfg.read_conf_operation_voltage();
    }

    void onSave() override {
        

        syscfg.set_conf_brightness(brightness);
        syscfg.set_conf_enable_alert(en_sos);
        syscfg.set_conf_enable_blink(en_led);
        syscfg.set_conf_enable_geiger_click(en_sound_click);
        syscfg.set_conf_enable_navi_tone(en_sound_navigate);

        syscfg.set_conf_warn_threshold(cpm_warn_threshold);
        syscfg.set_conf_dngr_threshold(cpm_dngr_threshold);
        syscfg.set_conf_hzdr_threshold(cpm_hzdr_threshold);

        operation_voltage = syscfg.read_conf_operation_voltage();

        syscfg.save_conf_to_nvs();
        // syscfg.set_conf_tube_convertion_coefficient();
    }
};

AppItem settings_app{
    .title = "Settings",
    .bitmap = image_settings_bits,
    
    .createApp = [](PixelUI& ui) -> std::unique_ptr<IApplication> { 
        return std::make_unique<APP_SETTINGS>(ui, itemList, 7); 
    },
};