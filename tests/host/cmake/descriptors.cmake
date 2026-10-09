# HID report descriptors the firmware advertises, checked against the report structs it sends.
# bt_descriptors.cpp and spice2x_device.cpp are the real sources, built against the fakes in
# support/descriptors/fakes; the other descriptors are built from the hid_reports.h macros.
file(GLOB SANTROLLER_DESCRIPTOR_TESTS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/descriptors/*_test.cpp)
add_executable(descriptors_tests
  ${SANTROLLER_DESCRIPTOR_TESTS}
  ${SANTROLLER_ROOT}/src/emulation/bt/bt_descriptors.cpp
  ${SANTROLLER_ROOT}/src/emulation/usb/spice2x_device.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/support/descriptors/spice2x_descriptor.cpp
)
# The fakes come first so they shadow the firmware's own headers
target_include_directories(descriptors_tests BEFORE PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support/descriptors/fakes)
target_include_directories(descriptors_tests PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/support
  ${CMAKE_CURRENT_SOURCE_DIR}/support/descriptors
)
# Tests read device sources and GATT databases to check the descriptor pairings
target_compile_definitions(descriptors_tests PRIVATE SANTROLLER_ROOT="${SANTROLLER_ROOT}")
target_compile_options(descriptors_tests PRIVATE -Wall -Wextra -Wno-missing-field-initializers -Wno-unused-parameter)
target_link_libraries(descriptors_tests PRIVATE santroller_hidparser GTest::gtest_main)
gtest_discover_tests(descriptors_tests DISCOVERY_MODE PRE_TEST)
