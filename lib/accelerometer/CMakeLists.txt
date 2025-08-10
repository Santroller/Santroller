add_library(accelerometer INTERFACE)
target_include_directories(accelerometer INTERFACE .)
target_link_libraries(accelerometer INTERFACE
    peripherals
)
target_sources(accelerometer INTERFACE accelerometerlib.cpp)