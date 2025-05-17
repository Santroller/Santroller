add_library(ws2812 ws2812.cpp)
target_include_directories(ws2812 INTERFACE .)
target_link_libraries(ws2812 PUBLIC
    pico_stdlib
    hardware_pio
    hardware_clocks
    hardware_timer
)
pico_generate_pio_header(ws2812 ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio)