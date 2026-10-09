# Wii extensions, both ways round: reading an extension plugged into us (lib/wii_extensions),
# pretending to be one for a Wii Remote (lib/wii_extension_emulation and the report building in
# src/emulation/wii_extension_input.cpp), the extension encryption (lib/wm_crypto) and the pure
# report packing of the Bluetooth Wii Remote emulation (lib/wii_remote_emulation/wm_reports.c).
# The I2C, alarm and GPIO calls go to the fakes in support/wii, which let a test play the other
# end of the bus. Building the emulated reports needs the real mappings, so this reuses the
# mappings area's fakes for the Pico clock, TinyUSB and the profile / MIDI device bits.
set(WII_MAPPING_SOURCES
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

file(GLOB SANTROLLER_WII_TESTS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/wii/*_test.cpp)
add_executable(wii_tests
  ${SANTROLLER_WII_TESTS}
  ${SANTROLLER_ROOT}/lib/wm_crypto/wm_crypto.c
  ${SANTROLLER_ROOT}/lib/wii_extension_emulation/wii_extension_backend.c
  ${SANTROLLER_ROOT}/lib/wii_extension_emulation/wii_extension_emulation.cpp
  ${SANTROLLER_ROOT}/lib/wii_extensions/wii_extension_decoder.cpp
  ${SANTROLLER_ROOT}/lib/wii_extensions/wii_extension.cpp
  ${SANTROLLER_ROOT}/lib/wii_remote_emulation/wm_reports.c
  ${SANTROLLER_ROOT}/src/emulation/wii_extension_input.cpp
  ${WII_MAPPING_SOURCES}
  ${CMAKE_CURRENT_SOURCE_DIR}/support/mappings/fakes.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/support/wii/fakes.cpp
)
# The fakes come first so they shadow the SDK / firmware headers; support/wii goes before
# support/mappings so its pico/time.h can add alarms on top of the mappings one
target_include_directories(wii_tests BEFORE PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/support/wii
  ${CMAKE_CURRENT_SOURCE_DIR}/support/mappings
)
target_include_directories(wii_tests PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/support
  ${SANTROLLER_ROOT}/lib/wii_extensions
  ${SANTROLLER_ROOT}/lib/wii_extension_emulation
  ${SANTROLLER_ROOT}/lib/wm_crypto
  ${SANTROLLER_ROOT}/lib/peripherals
  ${SANTROLLER_ROOT}/lib/wii_remote_emulation
  ${SANTROLLER_ROOT}/lib/xgip_protocol
)
target_compile_options(wii_tests PRIVATE -Wall -Wextra -fno-sanitize=vptr $<$<COMPILE_LANGUAGE:CXX>:-Wno-volatile> -Wno-missing-field-initializers -Wno-unused-parameter)
target_link_libraries(wii_tests PRIVATE santroller_headers GTest::gtest_main)
gtest_discover_tests(wii_tests DISCOVERY_MODE PRE_TEST)
