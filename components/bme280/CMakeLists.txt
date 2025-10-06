idf_component_register(
    SRCS "bme280_port.cpp" "bme280.c"
    INCLUDE_DIRS "."
    REQUIRES "driver" "freertos" "esp_common" "esp_timer"
)