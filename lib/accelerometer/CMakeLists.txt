add_library(accelerometer accelerometer.cpp)
target_include_directories(accelerometer INTERFACE .)
target_link_libraries(accelerometer PUBLIC
    peripherals
)