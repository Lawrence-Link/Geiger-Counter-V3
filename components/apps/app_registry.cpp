#include "app_registry.h"

extern AppItem counter_app;
extern AppItem settings_app;
extern AppItem app_penis;
extern AppItem cube_demo_app;
extern AppItem app_environment;
extern AppItem app_games;
extern AppItem racing_game_app;

void registerApps() {
    auto& app_man = AppManager::getInstance();
    app_man.registerApp(counter_app);
    app_man.registerApp(settings_app);
    app_man.registerApp(app_penis);
    app_man.registerApp(cube_demo_app);
    app_man.registerApp(app_environment);
    app_man.registerApp(app_games);
    app_man.registerApp(racing_game_app);
}