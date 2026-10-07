add_library(xbox360_rf INTERFACE)
target_include_directories(xbox360_rf INTERFACE .)
target_link_libraries(xbox360_rf INTERFACE
    pico_stdlib
    hardware_pio
    hardware_clocks
)
target_sources(xbox360_rf INTERFACE libxbox360_rf.cpp)
pico_generate_pio_header(xbox360_rf ${CMAKE_CURRENT_LIST_DIR}/xbox360_rf.pio)
