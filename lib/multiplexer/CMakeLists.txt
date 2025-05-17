add_library(multiplexer INTERFACE)
target_include_directories(multiplexer INTERFACE .)
target_link_libraries(multiplexer INTERFACE
    hardware_gpio
    hardware_adc
)
target_sources(multiplexer INTERFACE multiplexer.cpp)