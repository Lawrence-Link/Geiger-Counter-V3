#pragma once

#include "PixelUI.h"
#include "core/app/IApplication.h"
#include "core/ViewManager/ViewManager.h"
#include <memory>

class AppLauncher {
public:
    // Factory method: creates and returns a fully configured app launcher view.
    static std::shared_ptr<IApplication> createAppLauncherView(PixelUI& ui, ViewManager& viewManager);
};