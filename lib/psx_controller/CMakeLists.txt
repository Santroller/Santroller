add_library(psx_controller INTERFACE)
pico_generate_pio_header(psx_controller ${CMAKE_CURRENT_LIST_DIR}/psx_controller.pio)
target_include_directories(psx_controller INTERFACE . ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(psx_controller INTERFACE
    hardware_spi
    hardware_dma
    hardware_pio
    peripherals
)
target_sources(psx_controller INTERFACE psx_controller.cpp)
