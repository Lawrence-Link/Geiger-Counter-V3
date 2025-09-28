idf_component_register(
    SRCS "voltage_pid.cpp"
    INCLUDE_DIRS "."
    REQUIRES driver freertos esp_adc
)