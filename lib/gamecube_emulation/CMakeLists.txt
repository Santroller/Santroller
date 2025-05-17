# SPDX-License-Identifier: GPL-3.0-or-later
#
add_library(gamecube_emulation INTERFACE)
target_include_directories(gamecube_emulation INTERFACE ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(gamecube_emulation INTERFACE pico_stdlib hardware_pio hardware_gpio)
target_sources(gamecube_emulation INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/gamecube_device.c
    ${CMAKE_CURRENT_LIST_DIR}/gamecube_device.h)
pico_generate_pio_header(gamecube_emulation ${CMAKE_CURRENT_LIST_DIR}/joybus.pio)
