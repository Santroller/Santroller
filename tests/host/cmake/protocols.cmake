# Header-only protocol definitions and helpers from include/protocols/
file(GLOB SANTROLLER_PROTOCOL_TESTS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/protocols/*_test.cpp)
add_executable(protocols_tests
  ${SANTROLLER_PROTOCOL_TESTS}
  # Santroller 1 report decoding, shared by the BLE and USB hosts
  ${SANTROLLER_ROOT}/src/devices/santroller_v1_decode.cpp
  # XInput report decoding, shared by the XInput and Santroller 2 hosts
  ${SANTROLLER_ROOT}/src/devices/usb/host/xinput_tick_helpers.cpp
)
target_include_directories(protocols_tests PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/support
  # the fake clock, for the World Tour slider filter's millis()
  ${CMAKE_CURRENT_SOURCE_DIR}/support/mappings
  # xbox_one.hpp pulls in xgip_protocol.h
  ${SANTROLLER_ROOT}/lib/xgip_protocol
)
target_compile_options(protocols_tests PRIVATE -Wall -Wextra -Wno-missing-field-initializers -Wno-unused-parameter)
target_link_libraries(protocols_tests PRIVATE santroller_hidparser GTest::gtest_main)
gtest_discover_tests(protocols_tests DISCOVERY_MODE PRE_TEST)
