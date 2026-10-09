# Input modifiers (shortcut / held / toggle / cycle / shifted), profile activation triggers and the
# profile logic around them (shortcut masking, input queue, combined strum debounce, release to
# triggers). The real sources are compiled against the fakes in support/inputs, which sit in front
# of the mappings area's fakes (Pico clock, TinyUSB, HID config device events, bootrom) reused here.
set(INPUTS_SOURCES
  ${SANTROLLER_ROOT}/src/input/shortcut.cpp
  ${SANTROLLER_ROOT}/src/input/held.cpp
  ${SANTROLLER_ROOT}/src/input/toggle.cpp
  ${SANTROLLER_ROOT}/src/input/cycle.cpp
  ${SANTROLLER_ROOT}/src/input/shifted.cpp
  ${SANTROLLER_ROOT}/src/devices/toggle.cpp
  ${SANTROLLER_ROOT}/src/devices/cycle.cpp
  ${SANTROLLER_ROOT}/src/devices/device.cpp
  ${SANTROLLER_ROOT}/src/profiles/profile.cpp
  ${SANTROLLER_ROOT}/src/triggers/activation_trigger_list.cpp
  ${SANTROLLER_ROOT}/src/triggers/device_type_triggers.cpp
  ${SANTROLLER_ROOT}/src/triggers/input_trigger.cpp
  ${SANTROLLER_ROOT}/src/triggers/mode_triggers.cpp
  ${SANTROLLER_ROOT}/src/mappings/base_mapping.cpp
  ${SANTROLLER_ROOT}/src/managers/config_manager.cpp
)

file(GLOB SANTROLLER_INPUTS_TESTS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/inputs/*_test.cpp)
add_executable(inputs_tests
  ${INPUTS_SOURCES}
  ${SANTROLLER_INPUTS_TESTS}
  ${CMAKE_CURRENT_SOURCE_DIR}/support/inputs/fakes.cpp
)
# This area's fakes first, then the mappings area's, so both shadow the firmware's own headers
target_include_directories(inputs_tests BEFORE PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/support/inputs
  ${CMAKE_CURRENT_SOURCE_DIR}/support/mappings
)
target_include_directories(inputs_tests PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/support
  ${SANTROLLER_ROOT}/lib/wii_remote_emulation
  ${SANTROLLER_ROOT}/lib/wm_crypto
  ${SANTROLLER_ROOT}/lib/xgip_protocol
)
target_compile_options(inputs_tests PRIVATE -Wall -Wextra -fno-sanitize=vptr -Wno-volatile -Wno-missing-field-initializers -Wno-unused-parameter)
target_link_libraries(inputs_tests PRIVATE santroller_headers GTest::gtest_main)

gtest_discover_tests(inputs_tests DISCOVERY_MODE PRE_TEST)
