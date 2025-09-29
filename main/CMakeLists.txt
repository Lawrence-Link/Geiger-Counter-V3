idf_component_register(SRCS "main.cpp" "GPIO.cpp" "i2c.cpp" "battery.cpp" "encoder_task.cpp" "ui_heartbeat_task.cpp" "counter_task.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES "u8g2" "PixelUI" "voltage_pid" "cw2015chbd" "driver" "esp_timer" "freertos" "apps" "time_module")