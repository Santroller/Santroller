add_library(infinium_fader INTERFACE)
target_include_directories(infinium_fader INTERFACE .)
target_link_libraries(infinium_fader INTERFACE
    peripherals
)
target_sources(infinium_fader INTERFACE infinium_fader_device.cpp)