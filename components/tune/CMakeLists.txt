idf_component_register(
    SRCS "tune.cpp"
    INCLUDE_DIRS "."
    REQUIRES "driver" "esp_common" "main" buzzer
)