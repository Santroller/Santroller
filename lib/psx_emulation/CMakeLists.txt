add_library(psx_emulation psx_emulation.cpp)
target_include_directories(psx_emulation INTERFACE .)
target_link_libraries(psx_emulation PUBLIC
    peripherals
)
pico_generate_pio_header(psx_emulation ${CMAKE_CURRENT_LIST_DIR}/psxSPI.pio)