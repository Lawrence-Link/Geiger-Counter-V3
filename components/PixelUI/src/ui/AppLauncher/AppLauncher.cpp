#include "ui/AppLauncher/AppLauncher.h"
#include "ui/IconView/IconView.h"
#include "core/app/app_system.h"

std::shared_ptr<IApplication> AppLauncher::createAppLauncherView(PixelUI& ui, ViewManager& viewManager) {
    // 1. Create an IconView instance.
    auto iconView = std::make_shared<IconView>(ui);

    // 2. Configure the appearance and behavior of the IconView.
    iconView->setTitle("< Apps >");
    iconView->enableProgressBar(true);
    iconView->enableStatusText(true);
    iconView->enableSelectedItemTitle(true);

    // 3. Get the app list from AppManager and convert it to IconItems.
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

    // 4. Set the selection callback to launch the selected application.
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
    
    // 5. Return the fully configured view.
    return iconView;
}