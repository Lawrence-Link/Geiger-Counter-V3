# Build against the checked-out PixelUI release specified by the firmware
# workspace.  The legacy copy in this component is intentionally left
# untouched so the project never forks or modifies the framework itself.
set(PIXELUI_ROOT "/Users/link/dev/PixelUI")
file(GLOB_RECURSE SOURCES_CPPSRC CONFIGURE_DEPENDS
     "${PIXELUI_ROOT}/src/*.cpp")

idf_component_register(SRCS ${SOURCES_CPPSRC}
                    INCLUDE_DIRS "${PIXELUI_ROOT}/include"
                    REQUIRES "u8g2" "etl" "apps")

target_compile_options(${COMPONENT_LIB} PRIVATE 
    -fno-function-sections 
    -fno-data-sections
)
target_link_options(${COMPONENT_LIB} PRIVATE 
    "LINKER:--no-gc-sections"
)
target_compile_definitions(${COMPONENT_LIB} PRIVATE ESP_PLATFORM=1)
