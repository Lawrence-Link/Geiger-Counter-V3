/*
 * Copyright (C) 2025 Lawrence Link
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "ui/AppLauncher/AppLauncher.h"
#include "ui/IconView/IconView.h"
#include "core/app/app_system.h"

namespace {

class AppLauncherView final : public IconView<MAX_APP_NUM> {
public:
    AppLauncherView(PixelUI& ui, ViewManager& viewManager)
        : IconView<MAX_APP_NUM>(ui), viewManager_(viewManager) {}

/** @brief onEnter. */
    void onEnter(ExitCallback exitCallback) override {
        setTitle("< Apps >");
        enableProgressBar(true);
        enableStatusText(true);
        enableSelectedItemTitle(true);

        const auto& apps = AppManager::getInstance().getAppVector();
        IconItemList<MAX_APP_NUM> iconItems;
        for (const auto& app : apps) {
            iconItems.emplace_back(app.title, app.bitmap, const_cast<AppItem*>(&app));
        }
        setItems(iconItems);

        setSelectionCallback([this](int index, const IconItem& item) {
            (void)index;
            const AppItem* appItem = static_cast<const AppItem*>(item.userData);
            if (appItem != nullptr) {
                viewManager_.launch(*appItem);
            }
        });

        IconView<MAX_APP_NUM>::onEnter(exitCallback);
    }

private:
    ViewManager& viewManager_;
};

} // namespace

/** @brief AppLauncher::launch. */
ViewManager::LaunchResult AppLauncher::launch(PixelUI& ui, ViewManager& viewManager) {
    return viewManager.push<AppLauncherView>(ui, viewManager);
}
