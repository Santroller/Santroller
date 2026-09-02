add_library(wii_extension_emulation INTERFACE)
target_include_directories(wii_extension_emulation INTERFACE .)
target_link_libraries(wii_extension_emulation INTERFACE
    pico_i2c_slave
    hardware_i2c
    wm_crypto
)
target_sources(wii_extension_emulation INTERFACE
    wii_extension_emulation.cpp
    wii_extension_backend.c)