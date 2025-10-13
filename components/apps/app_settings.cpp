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
#include "core/ViewManager/ViewManager.h"
#include "ui/ListView/ListView.h"
#include "system_nvs_varibles.h"
#include "esp_log.h"
#include "voltage_pid.hpp"

static bool en_sound_click; // geiger click
static bool en_sound_navigate; // sound navigate
static bool en_sos = false;    // alert
static bool en_led = true;     // led
static bool en_interaction_tone;

static bool use_cpm;

static float convertion_coefficient = 0.0f;
static int32_t brightness = 0;
static int32_t cpm_warn_threshold = 0;
static int32_t cpm_dngr_threshold = 0;
static int32_t cpm_hzdr_threshold = 0;
static int32_t operation_voltage = 380;

static float vkp;
static float vki;
static float vkd;

static char str_coeff[12];

__attribute__((aligned(4)))
static const unsigned char image_settings_bits[] = {0xf0,0xff,0x0f,0xfc,0xff,0x3f,0xfe,0xff,0x7f,0xfe,0xe7,0x7f,0xff,0xe7,0xff,0x9f,0x81,0xf9,0x1f,0x3c,0xf8,0x3f,0xff,0xfc,0xbf,0xc3,0xfd,0x9f,0x3d,0xf9,0xdf,0xfe,0xff,0xc7,0x7e,0xe8,0xc7,0xbe,0xeb,0xdf,0x7e,0xfb,0x9f,0xed,0xf6,0xbf,0xd5,0xf6,0x3f,0x37,0xf7,0x1f,0xf4,0xef,0x9f,0xc5,0xdf,0xff,0x3f,0xbe,0xfe,0xe7,0x7d,0xfe,0xff,0x7b,0xfc,0xff,0x37,0xf0,0xff,0x0f};

extern PixelUI ui;
extern AppItem time_setting_app;
extern VoltagePID voltage_controller;

auto& syscfg = SystemConf::getInstance();

ListItem sub_Alarm[5] = {
    ListItem(">>> 剂量警告 <<<"),
    ListItem{.title = "- 启用", .extra = {.switchValue=&en_sos}},
    ListItem{.title = "- 警告阈值(CPM)",
            .pFunc = [](){ ui.showPopupValue4Digits(cpm_warn_threshold, "警告CPM", 100, 40, 3000, 1); }, 
            .extra = ListItemExtra{.intValue = &cpm_warn_threshold}},
    ListItem{
        .title = "- 危险阈值(CPM)",
        .pFunc = [](){ ui.showPopupValue4Digits(cpm_dngr_threshold, "危险CPM", 100, 40, 3000, 1); }, 
        .extra = ListItemExtra{.intValue = &cpm_dngr_threshold}},
    ListItem{.title = "- 灾难阈值(CPM)",
        .pFunc = [](){ ui.showPopupValue4Digits(cpm_hzdr_threshold, "灾难CPM", 100, 40, 3000, 1); }, 
        .extra = ListItemExtra{.intValue = &cpm_hzdr_threshold}}
};

ListItem sub_cvs_cfg[4] = {
    ListItem(">>> 恒压PID调试 <<<"),
    ListItem{.title = "- Kp 比例", .extra = {.float_dot1f_Value = &vkp}},
    ListItem{.title = "- Ki 积分", .extra = {.float_dot1f_Value = &vki}},
    ListItem{.title = "- Kd 微分", .extra = {.float_dot1f_Value = &vkd}}
};

ListItem sub_Tube_cfg[4] = {
    ListItem(">>> 盖革管 <<<"),
    ListItem{
        .title = "- 工作电压", 
        .pFunc = [](){ ui.showPopupProgress(operation_voltage, 340, 400, "工作电压", 100, 40, 5000, 1, nullptr, true);}, 
        .extra = ListItemExtra{.intValue = &operation_voltage}
    },
    ListItem{.title = "- 转换系数", .extra = {.text = str_coeff}},
    ListItem{.title = "- 恒压调试", .nextList = sub_cvs_cfg, .nextListLength = 4}
};

ListItem itemList[10] = {
    ListItem(">>>> 设置 <<<<"),
    ListItem{
        .title = "- 屏幕亮度",
        .pFunc = [](){ ui.showPopupProgress(brightness, 0, 5, "亮度", 100, 40, 5000, 1, [](int32_t val){ui.getU8G2().setContrast(val * 51);}); },
        .extra = ListItemExtra{
            .intValue = &brightness
        }
    },
    ListItem{
        .title = "- 剂量警告",
        .nextList = sub_Alarm,
        .nextListLength = 5
    },
    ListItem{
        .title = "- 盖革管",
        .nextList = sub_Tube_cfg,
        .nextListLength = 4
    },
    ListItem{
        .title = "- 使用CPM",
        .extra = ListItemExtra{
            .switchValue = &use_cpm
        }
    },
    ListItem{
        .title = "- 计数音",
        .extra = ListItemExtra{
            .switchValue = &en_sound_click
        }
    },
    ListItem{
        .title = "- 交互音",
        .extra = ListItemExtra{
            .switchValue = &en_interaction_tone
        }
    },
    ListItem{
        .title = "- 导航音",
        .pFunc = [](){syscfg.set_conf_enable_navi_tone(en_sound_navigate);},
        .extra = ListItemExtra{
            .switchValue = &en_sound_navigate
        }
    },
    ListItem{
        .title = "- RTC时间",
        .pFunc = [](){ ui.getViewManagerPtr()->push(time_setting_app.createApp(ui)); },
        .use_fade = true
    },
    ListItem{
        .title = "- LED指示",
        .extra = ListItemExtra{
            .switchValue = &en_led
        }
    }
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
        en_interaction_tone = syscfg.read_conf_enable_interaction_tone();
        
        vkp = syscfg.read_conf_volt_pid_kp();
        vki = syscfg.read_conf_volt_pid_ki();
        vkd = syscfg.read_conf_volt_pid_kd();

        use_cpm = syscfg.read_conf_use_cpm();

        sprintf(str_coeff, "%.6f", convertion_coefficient);
    }

    void onSave() override {
        syscfg.set_conf_brightness(brightness);
        syscfg.set_conf_enable_alert(en_sos);
        syscfg.set_conf_enable_blink(en_led);
        syscfg.set_conf_enable_geiger_click(en_sound_click);
        syscfg.set_conf_enable_navi_tone(en_sound_navigate);
        syscfg.set_conf_enable_interaction_tone(en_interaction_tone);

        syscfg.set_conf_warn_threshold(cpm_warn_threshold);
        syscfg.set_conf_dngr_threshold(cpm_dngr_threshold);
        syscfg.set_conf_hzdr_threshold(cpm_hzdr_threshold);

        syscfg.set_conf_use_cpm(use_cpm);
        // operation_voltage = syscfg.read_conf_operation_voltage();
        syscfg.set_conf_operation_voltage(operation_voltage);
        syscfg.save_conf_to_nvs();
        voltage_controller.setTarget(operation_voltage);
        // syscfg.set_conf_tube_convertion_coefficient();
    }
};

AppItem settings_app{
    .title = "设置",
    .bitmap = image_settings_bits,
    
    .createApp = [](PixelUI& ui) -> std::unique_ptr<IApplication> { 
        return std::make_unique<APP_SETTINGS>(ui, itemList, 10); 
    },
};