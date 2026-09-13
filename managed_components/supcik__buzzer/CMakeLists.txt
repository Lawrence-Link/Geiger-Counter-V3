include($ENV{IDF_PATH}/tools/cmake/version.cmake)

set(srcs "src/buzzer.cpp")

idf_component_register(SRCS ${srcs}
                    REQUIRES "driver" "esp_driver_ledc" "esp_driver_gpio"
                    INCLUDE_DIRS "include")
