idf_component_register(
    SRCS "time_module.c"
    INCLUDE_DIRS "."
    REQUIRES "driver" "esp_common" "main" log esp_idf_lib_helpers freertos esp_driver_i2c
)

set_property(TARGET ${COMPONENT_LIB} APPEND PROPERTY LINK_INTERFACE_MULTIPLICITY 3)
