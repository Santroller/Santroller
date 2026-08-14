add_library(psx_emulation INTERFACE)
target_include_directories(psx_emulation INTERFACE .)
target_link_libraries(psx_emulation INTERFACE
    peripherals
)
pico_generate_pio_header(psx_emulation ${CMAKE_CURRENT_LIST_DIR}/pio_spi.pio)
target_sources(psx_emulation INTERFACE psx_emulation.cpp pio_spi.c)