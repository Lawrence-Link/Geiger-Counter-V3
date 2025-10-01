idf_component_register(
    SRCS "GPIO.cpp"
    INCLUDE_DIRS "."
    REQUIRES "driver" "freertos" "main"
)