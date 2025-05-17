add_library(multiplexer multiplexer.cpp)
target_include_directories(multiplexer INTERFACE .)
target_link_libraries(multiplexer PUBLIC
    hardware_gpio
    hardware_adc
)