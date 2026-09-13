idf_component_register(
    SRCS "GPIO.cpp"
    INCLUDE_DIRS "."
    REQUIRES "driver" "freertos" "main" "tune" "esp_driver_ledc" "esp_driver_gpio"
)
