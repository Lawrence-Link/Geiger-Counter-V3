idf_component_register(
    SRCS "counter_task.cpp"
    INCLUDE_DIRS "."
    REQUIRES "driver" "freertos" "esp_timer" "tune" "led"
)