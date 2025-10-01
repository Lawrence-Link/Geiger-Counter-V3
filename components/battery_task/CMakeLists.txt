idf_component_register(
    SRCS "battery.cpp"
    INCLUDE_DIRS "."
    REQUIRES "driver" "freertos" "cw2015chbd" "main"
)