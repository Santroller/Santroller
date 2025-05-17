add_library(matrix INTERFACE)
target_include_directories(matrix INTERFACE .)
target_link_libraries(matrix INTERFACE
    hardware_gpio
)
target_sources(matrix INTERFACE matrix.cpp)