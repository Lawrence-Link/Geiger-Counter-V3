idf_component_register(
    SRCS "voltage_pid.cpp"
    INCLUDE_DIRS "."
    REQUIRES driver freertos esp_adc esp_driver_ledc esp_driver_gpio
)
