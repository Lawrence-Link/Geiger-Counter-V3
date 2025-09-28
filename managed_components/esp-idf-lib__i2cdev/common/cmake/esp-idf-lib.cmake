# Set common build flags but enable them only when the build is in our CI,
# making them optional. The idea is, fail when compiled in our CI but just
# warn in any other cases. Code that compiles with one toolchain warning-free
# may not do so with another toolchain, which creates a project dependency on
# specific toolchain vendors and versions.
#
# Define flags that may cause failures here.
if (DEFINED ENV{ESP_IDF_LIB_CI})
    set(ESP_IDF_LIB_CI_FLAGS
        -Werror=unused-variable
        -Werror=unused-function
        -Werror=write-strings
        -Werror
    )
endif()

# Set common build flags. Mandatory.
#
# Define flags that do not cause failures here.
set(ESP_IDF_LIB_FLAGS
    -Wextra
    -Wwrite-strings
    -Wunused-variable
    -Wunused-function
    -Wreturn-type
)

# When COMPONENT_LIB is INTERFACE_LIBRARY, or a header-only library, do not
# set the flags.
get_target_property(COMPONENT_TYPE ${COMPONENT_LIB} TYPE)
if(NOT COMPONENT_TYPE STREQUAL "INTERFACE_LIBRARY")
    target_compile_options(${COMPONENT_LIB} PRIVATE
        ${ESP_IDF_LIB_FLAGS}
        ${ESP_IDF_LIB_CI_FLAGS}
    )
endif()
