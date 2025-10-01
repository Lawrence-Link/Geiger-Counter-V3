include($ENV{IDF_PATH}/tools/cmake/version.cmake)

set(srcs "src/buzzer.cpp")

idf_component_register(SRCS ${srcs}
                    REQUIRES "driver"
                    INCLUDE_DIRS "include")
