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
#pragma once
#include <functional>
#include <stdint.h>
#include <etl/vector.h>
#include "IApplication.h"
#include "config.h"
#include <algorithm>
#include <memory>

enum class MenuItemType {
    Action,
    App,
};

struct AppItem {
    const char* title;
    const uint8_t* bitmap;
    std::function<std::shared_ptr<IApplication>(PixelUI&)> createApp;
};

class AppManager {
public:
    static AppManager& getInstance() {
        static AppManager instance;
        return instance;
    }
    void registerApp(const AppItem& item);
    const etl::vector<AppItem, MAX_APP_NUM>& getAppVector() const;
    size_t getRegisteredCount() const { return appItems_.size(); }
    AppManager(const AppManager&) = delete;
    AppManager& operator=(const AppManager&) = delete;
private:
    AppManager() = default;
    etl::vector<AppItem, MAX_APP_NUM> appItems_;
};