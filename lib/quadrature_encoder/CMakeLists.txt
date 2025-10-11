add_library(quadrature_encoder INTERFACE)
target_include_directories(quadrature_encoder INTERFACE .)
target_link_libraries(quadrature_encoder INTERFACE
    hardware_gpio
    hardware_pio
)
target_sources(quadrature_encoder INTERFACE quadrature_encoder.cpp)
pico_generate_pio_header(ws2812 ${CMAKE_CURRENT_LIST_DIR}/quadrature_encoder.pio)