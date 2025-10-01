idf_component_register(
    SRCS "led.cpp"
    INCLUDE_DIRS "."
    REQUIRES "driver" "freertos" "gpio_stuff"
)