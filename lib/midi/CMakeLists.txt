add_library(midi INTERFACE)
target_include_directories(midi INTERFACE .)
target_sources(midi INTERFACE MIDI.cpp SimpleMidiTransport.cpp)