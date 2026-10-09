# Output mappings: the real src/mappings sources, compiled against the fakes in support/mappings
# (Pico time / bootrom, TinyUSB without its Pico OS layer, the HID config device and the
# profile / instance headers the mappings only include for their types)
set(MAPPINGS_SOURCES
  ${SANTROLLER_ROOT}/src/mappings/base_mapping.cpp
  ${SANTROLLER_ROOT}/src/mappings/gamepad_mapping.cpp
  ${SANTROLLER_ROOT}/src/mappings/red_octane_mappings.cpp
  ${SANTROLLER_ROOT}/src/mappings/rock_band_mappings.cpp
  ${SANTROLLER_ROOT}/src/mappings/konami_mappings.cpp
  ${SANTROLLER_ROOT}/src/mappings/djmax_mappings.cpp
  ${SANTROLLER_ROOT}/src/mappings/project_diva_mappings.cpp
  ${SANTROLLER_ROOT}/src/mappings/taiko_mappings.cpp
  ${SANTROLLER_ROOT}/src/mappings/peripheral_mappings.cpp
  ${SANTROLLER_ROOT}/src/mappings/powergig_mappings.cpp
  ${SANTROLLER_ROOT}/src/mappings/rock_revolution_mappings.cpp
)

add_executable(mappings_tests
  ${MAPPINGS_SOURCES}
  ${CMAKE_CURRENT_SOURCE_DIR}/support/mappings/fakes.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mappings/calibration_test.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mappings/base_mapping_test.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mappings/gamepad_mapping_test.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mappings/guitar_mapping_test.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mappings/drums_mapping_test.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/mappings/arcade_mapping_test.cpp
)
# The fakes come first so they shadow the firmware's own headers
target_include_directories(mappings_tests BEFORE PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support/mappings)
target_include_directories(mappings_tests PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/support
  ${SANTROLLER_ROOT}/lib/wii_remote_emulation
  ${SANTROLLER_ROOT}/lib/wm_crypto
  ${SANTROLLER_ROOT}/lib/xgip_protocol
)
target_compile_options(mappings_tests PRIVATE -Wall -Wextra -fno-sanitize=vptr -Wno-volatile -Wno-missing-field-initializers -Wno-unused-parameter)
target_link_libraries(mappings_tests PRIVATE santroller_headers GTest::gtest_main)

gtest_discover_tests(mappings_tests DISCOVERY_MODE PRE_TEST)
