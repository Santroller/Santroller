add_library(stp16cpc INTERFACE)
target_include_directories(stp16cpc INTERFACE .)
target_link_libraries(stp16cpc INTERFACE
    pico_stdlib
    peripherals
    hardware_clocks
    hardware_timer
)
target_sources(ws2812 INTERFACE libstp16cpc.cpp)