add_library(apa102 INTERFACE)
target_include_directories(apa102 INTERFACE .)
target_link_libraries(apa102 INTERFACE
    pico_stdlib
    peripherals
    hardware_clocks
    hardware_timer
)
target_sources(apa102 INTERFACE libapa102.cpp)