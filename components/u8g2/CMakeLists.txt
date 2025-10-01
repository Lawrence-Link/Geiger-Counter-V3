file(GLOB_RECURSE SOURCES_CPPSRC "cppsrc/*.cpp")

# 收集 csrc 目录下的所有 .c 文件
file(GLOB_RECURSE SOURCES_CSRC "csrc/*.c")

idf_component_register(
    SRCS ${SOURCES_CPPSRC} ${SOURCES_CSRC}
    INCLUDE_DIRS "cppsrc" "csrc"
    REQUIRES "driver" "esp_common" 
)
