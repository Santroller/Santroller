# MIDI input parsing (include/protocols/midi_input.hpp), which MidiDevice wraps
file(GLOB SANTROLLER_MIDI_TESTS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/midi/*_test.cpp)
add_executable(midi_tests ${SANTROLLER_MIDI_TESTS})
target_include_directories(midi_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support)
target_compile_options(midi_tests PRIVATE -Wall -Wextra -Wno-missing-field-initializers -Wno-unused-parameter)
target_link_libraries(midi_tests PRIVATE santroller_headers GTest::gtest_main)
gtest_discover_tests(midi_tests DISCOVERY_MODE PRE_TEST)
