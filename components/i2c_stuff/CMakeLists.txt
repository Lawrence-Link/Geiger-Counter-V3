idf_component_register(
    SRCS "i2c.cpp"
    INCLUDE_DIRS "."
    REQUIRES "driver" "freertos" "cw2015chbd" "gpio_stuff" "bme280")