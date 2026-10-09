# SPDX-License-Identifier: GPL-3.0-or-later
#
add_library(joybus_emulation INTERFACE)
target_include_directories(joybus_emulation INTERFACE ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(joybus_emulation INTERFACE pico_stdlib hardware_pio hardware_gpio hardware_dma hardware_irq)
target_sources(joybus_emulation INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/libjoybus_emulation.cpp)
pico_generate_pio_header(joybus_emulation ${CMAKE_CURRENT_LIST_DIR}/joybus.pio)
