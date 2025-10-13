# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 为所有 C 编译启用 LTO
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${LTO_FLAGS}")

# 为所有 C++ 编译启用 LTO
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${LTO_FLAGS}")

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${LTO_FLAGS}")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${LTO_FLAGS}")


include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(GeigerCounter)