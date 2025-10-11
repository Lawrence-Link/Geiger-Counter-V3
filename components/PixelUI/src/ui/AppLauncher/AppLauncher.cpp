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

#include "ui/AppLauncher/AppLauncher.h"
#include "ui/IconView/IconView.h"
#include "core/app/app_system.h"

std::shared_ptr<IApplication> AppLauncher::createAppLauncherView(PixelUI& ui, ViewManager& viewManager) {
    // Create an IconView instance.
    auto iconView = std::make_shared<IconView>(ui);

    // Configure the appearance and behavior of the IconView.
    iconView->setTitle("< Apps >");
    iconView->enableProgressBar(true);
    iconView->enableStatusText(true);
    iconView->enableSelectedItemTitle(true);

    // Get the app list from AppManager and convert it to IconItems.
    auto& appManager = AppManager::getInstance();
    const auto& apps = appManager.getAppVector();
    std::vector<IconItem> iconItems;
    iconItems.reserve(apps.size()); // Pre-allocate memory.
    for (const auto& app : apps) {
        // We use const_cast because AppItem* will be stored in a void*,
        // but we guarantee it will not be modified.
        iconItems.emplace_back(app.title, app.bitmap, const_cast<AppItem*>(&app));
    }
    iconView->setItems(iconItems);

    // Set the selection callback to launch the selected application.
    iconView->setSelectionCallback(
        [&ui, &viewManager](int index, const IconItem& item) {
            const AppItem* appItem = static_cast<const AppItem*>(item.userData);
            if (appItem && appItem->createApp) {
                auto appInstance = appItem->createApp(ui);
                if (appInstance) {
                    viewManager.push(appInstance);
                }
            }
        }
    );
    
    // Return the fully configured view.
    return iconView;
}