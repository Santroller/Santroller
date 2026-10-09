# Xbox One GIP: the packet framing / fragmentation in lib/xgip_protocol, the shared host side
# controller handling in lib/gip_common, and both of their users, the USB host driver for
# Xbox One controllers (src/devices/usb/host/xone_host.cpp) and the emulated Xbox One device
# (src/emulation/usb/xone_device.cpp). Those two are compiled against the fakes in support/gip,
# which record the USB transfers so a test can play the console or the controller.
file(GLOB SANTROLLER_GIP_TESTS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/gip/*_test.cpp)
add_executable(gip_tests
  ${SANTROLLER_GIP_TESTS}
  ${SANTROLLER_ROOT}/lib/xgip_protocol/xgip_protocol.cpp
  ${SANTROLLER_ROOT}/lib/gip_common/gip_device.cpp
  ${SANTROLLER_ROOT}/lib/gip_common/gip_packet_handler.cpp
  ${SANTROLLER_ROOT}/lib/gip_common/gip_button_mapping.cpp
  ${SANTROLLER_ROOT}/lib/gip_common/gip_report_queue.cpp
  ${SANTROLLER_ROOT}/src/usb/auth_broker.cpp
  ${SANTROLLER_ROOT}/src/devices/usb/host/xone_host.cpp
  ${SANTROLLER_ROOT}/src/emulation/usb/xone_device.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/support/gip/fakes.cpp
)
# The fakes come first so they shadow the SDK / firmware headers
target_include_directories(gip_tests BEFORE PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support/gip)
target_include_directories(gip_tests PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/support
  ${SANTROLLER_ROOT}/lib/xgip_protocol
  ${SANTROLLER_ROOT}/lib/gip_common
)
# Some tests read the emulated device's descriptor tables out of its source
target_compile_definitions(gip_tests PRIVATE SANTROLLER_ROOT="${SANTROLLER_ROOT}")
target_compile_options(gip_tests PRIVATE -Wall -Wextra -fno-sanitize=vptr -Wno-volatile -Wno-missing-field-initializers -Wno-unused-parameter)
# On the Pico these come in through the SDK headers; the sources don't include them themselves
set_source_files_properties(
  ${SANTROLLER_ROOT}/lib/xgip_protocol/xgip_protocol.cpp
  ${SANTROLLER_ROOT}/src/emulation/usb/xone_device.cpp
  PROPERTIES COMPILE_OPTIONS "-include;cstddef;-include;cassert"
)
target_link_libraries(gip_tests PRIVATE santroller_headers GTest::gtest_main)
gtest_discover_tests(gip_tests DISCOVERY_MODE PRE_TEST)
