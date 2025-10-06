idf_component_register(SRCS "app_settings_utils/app_time_setting.cpp" 
"app_counter.cpp" "app_settings.cpp" "app_penis.cpp" "app_environment.cpp" "app_registry.cpp" "app_ui_utils/app_charging_animation.cpp" 
"app_cube.cpp" "app_ui_utils/app_silly_cat.cpp" "app_games.cpp" "racing_game.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES "PixelUI" "voltage_pid" "main" "time_module" "counter_task" "system_nvs_varibles" "led" "bme280")

# 防止静态注册被优化
target_compile_options(${COMPONENT_LIB} PRIVATE 
    -fno-function-sections 
    -fno-data-sections
)

target_link_options(${COMPONENT_LIB} PRIVATE 
    "LINKER:--no-gc-sections"
)
