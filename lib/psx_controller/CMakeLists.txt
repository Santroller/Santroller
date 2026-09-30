add_library(psx_controller INTERFACE)
target_include_directories(psx_controller INTERFACE .)
target_link_libraries(psx_controller INTERFACE
    peripherals
)
pico_generate_pio_header(psx_controller ${CMAKE_CURRENT_LIST_DIR}/psx_controller.pio)
target_sources(psx_controller INTERFACE psx_controller.cpp)
