idf_component_register(
    SRCS "data_storage.cpp" "data_storage_core.cpp"
    INCLUDE_DIRS "."
    REQUIRES "littlefs" "freertos")
