# The radio firmware area (src/radio_firmware.cpp): reading its table of contents or the older layout, and
# writing a new image the way the configurator does, against NOR flash in RAM (support/radio)
add_executable(radio_tests
  ${SANTROLLER_ROOT}/src/radio_firmware.cpp
  ${SANTROLLER_ROOT}/lib/uzlib/src/crc32.c
  ${CMAKE_CURRENT_SOURCE_DIR}/support/radio/fakes.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/radio/radio_firmware_test.cpp
)
# The fakes come first so they shadow the firmware's own headers
target_include_directories(radio_tests BEFORE PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support/radio)
target_include_directories(radio_tests PRIVATE ${SANTROLLER_ROOT}/include ${SANTROLLER_ROOT}/lib/uzlib/src)
target_compile_definitions(radio_tests PRIVATE ENABLE_XBOX_WIRELESS=1)
target_compile_options(radio_tests PRIVATE -Wall -Wextra -Wno-missing-field-initializers -Wno-unused-parameter)
target_link_libraries(radio_tests PRIVATE GTest::gtest_main)

gtest_discover_tests(radio_tests DISCOVERY_MODE PRE_TEST)
