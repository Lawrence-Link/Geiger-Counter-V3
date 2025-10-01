idf_component_register(
                    SRCS 
                    "main.cpp" 
               
                    INCLUDE_DIRS "."
                    
                    REQUIRES 
                    "u8g2" 
                    "PixelUI" 
                    "voltage_pid" 
                    "driver" 
                    "esp_timer" 
                    "freertos" 
                    "apps" 
                    "time_module"
                    "battery_task"
                    "encoder_task"
                    "ui_heartbeat_task"
                    "gpio_stuff"
                    "i2c_stuff"
                    "system_nvs_varibles"
                    "nvs"
                    "tune"
                    )