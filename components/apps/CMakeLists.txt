idf_component_register(SRCS "app_counter.cpp" "app_settings.cpp" "app_penis.cpp" "app_registry.cpp" "app_charging_animation.cpp" "app_cube.cpp" "app_silly_cat.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES "PixelUI" "voltage_pid" "main" "time_module")

# 防止静态注册被优化
target_compile_options(${COMPONENT_LIB} PRIVATE 
    -fno-function-sections 
    -fno-data-sections
)

target_link_options(${COMPONENT_LIB} PRIVATE 
    "LINKER:--no-gc-sections"
)
