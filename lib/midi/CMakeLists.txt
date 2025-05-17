add_library(midi MIDI.cpp SimpleMidiTransport.cpp MIDIData.cpp)
target_include_directories(midi INTERFACE .)