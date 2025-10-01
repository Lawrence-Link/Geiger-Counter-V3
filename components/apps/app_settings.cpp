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
#include "system_varibles.h"

bool en_sound_click = false;
bool en_sos = false;

int32_t brightness = 0;
int32_t sound_volume = 0;

static const unsigned char image_LISTVIEW_bits[] = {0xf0,0xff,0x0f,0xfc,0xff,0x3f,0xfe,0xff,0x7f,0xfe,0xe7,0x7f,0xff,0xe7,0xff,0x9f,0x81,0xf9,0x1f,0x3c,0xf8,0x3f,0xff,0xfc,0xbf,0xc3,0xfd,0x9f,0x3d,0xf9,0xdf,0xfe,0xff,0xc7,0x7e,0xe8,0xc7,0xbe,0xeb,0xdf,0x7e,0xfb,0x9f,0xed,0xf6,0xbf,0xd5,0xf6,0x3f,0x37,0xf7,0x1f,0xf4,0xef,0x9f,0xc5,0xdf,0xff,0x3f,0xbe,0xfe,0xe7,0x7d,0xfe,0xff,0x7b,0xfc,0xff,0x37,0xf0,0xff,0x0f};

extern PixelUI ui;

ListItem sub_Language[3] = {
    ListItem(">>> Language <<<"),
    ListItem("- ENGLISH", nullptr, 0, [](){  }),
    ListItem("- 简体中文", nullptr, 0, [](){  })
};

ListItem sub_Alarm[3] = {
    ListItem(">>> Alarm <<<"),
    ListItem("- Enable", nullptr, 0, nullptr, ListItemExtra{&en_sos, nullptr}),
    ListItem("- Warn at", nullptr, 0, [](){  })
};

// ListItem itemList[10] = {
//     ListItem(">>> ListDemo <<<"),
//     ListItem("- Show pop", nullptr, 0, [](){ ui.showPopupInfo("Hello from PixelUI!", "Info", 80, 30, 2000); }),
//     ListItem("- Sub Menu", sub_CathyFlower, 3),
//     ListItem("- Bool State", nullptr, 0, nullptr, ListItemExtra{&bool_state, nullptr}),
//     ListItem("- Value", nullptr, 0, [](){ ui.showPopupProgress(my_value, 0, 100, "Value", 100, 40, 5000, 1); }, ListItemExtra{nullptr, &my_value}),
//     ListItem("- Alert", nullptr, 0, [](){  }),
//     ListItem("- Progress", nullptr, 0, [](){  }),
//     ListItem("- Anytone", nullptr, 0, [](){  }),
//     ListItem("- Potato", nullptr, 0, [](){  }),
//     ListItem("- Tomato", nullptr, 0, [](){  })
// };

ListItem itemList[10] = {
    ListItem(">>>> 设置 <<<<"),
    ListItem("- 屏幕亮度", nullptr, 0, [](){ ui.showPopupProgress(brightness, 0, 5, "亮度", 100, 40, 5000, 1, [](int32_t val){ui.getU8G2().setContrast(val * 51);}); }, ListItemExtra{nullptr, &brightness}),
    ListItem("- 音量", nullptr, 0, [](){ ui.showPopupProgress(sound_volume, 0, 100, "音量", 100, 40, 5000, 1); }, ListItemExtra{nullptr, &sound_volume}),
    ListItem("- LED", nullptr, 0, [](){  }),
    ListItem("- 盖革管", nullptr, 0, [](){  }),
    ListItem("- 辐射警示", nullptr, 0, nullptr, ListItemExtra{&en_sound_click, nullptr}),
    ListItem("- 辐射警报", sub_Alarm, 3),
    ListItem("- Anytone", nullptr, 0, [](){  }),
    ListItem("- Potato", nullptr, 0, [](){  }),
    ListItem("- Tomato", nullptr, 0, [](){  }), 
};

class APP_SETTINGS : public ListView {
public:
    APP_SETTINGS(PixelUI& ui, ListItem *itemList, size_t length) : ListView(ui, itemList, length) {
        brightness = SystemConf::getInstance().read_conf_brightness();
    }
    void onSave() override {
        
    }
};

AppItem settings_app{
    .title = "Settings",
    .bitmap = image_LISTVIEW_bits,
    
    .createApp = [](PixelUI& ui) -> std::unique_ptr<IApplication> { 
        return std::make_unique<APP_SETTINGS>(ui, itemList, 10); 
    },
};