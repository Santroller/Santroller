add_library(matrix matrix.cpp)
target_include_directories(matrix INTERFACE .)
target_link_libraries(matrix PUBLIC
    hardware_gpio
)