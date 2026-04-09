add_library(midi_serial INTERFACE)
target_include_directories(midi_serial INTERFACE .)
target_link_libraries(midi_serial INTERFACE
    peripherals
)
target_sources(midi_serial INTERFACE midi_serial.cpp)