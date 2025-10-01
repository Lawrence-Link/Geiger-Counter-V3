file(GLOB_RECURSE SOURCES_CPPSRC "*.cpp")

idf_component_register(SRCS ${SOURCES_CPPSRC}
                    INCLUDE_DIRS "./include"
                    REQUIRES "u8g2" "etl" "apps")

target_compile_options(${COMPONENT_LIB} PRIVATE 
    -fno-function-sections 
    -fno-data-sections
)
target_link_options(${COMPONENT_LIB} PRIVATE 
    "LINKER:--no-gc-sections"
)
target_compile_definitions(${COMPONENT_LIB} PRIVATE 
    ESP_PLATFORM=1
)