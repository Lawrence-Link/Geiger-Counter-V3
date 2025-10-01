idf_component_register(
    SRCS "ui_heartbeat_task.cpp"
    INCLUDE_DIRS "."
    REQUIRES "driver" "freertos" "PixelUI"
)