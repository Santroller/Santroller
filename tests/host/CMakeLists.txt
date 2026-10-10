# Host-native unit tests for the firmware logic that doesn't need real hardware.
#
#   cmake -S tests/host -B build-tests -G Ninja
#   cmake --build build-tests
#   ctest --test-dir build-tests --output-on-failure
cmake_minimum_required(VERSION 3.24)
project(santroller_host_tests C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

get_filename_component(SANTROLLER_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../.. ABSOLUTE)

option(SANTROLLER_TESTS_SANITIZE "Build the tests with address and undefined behaviour sanitizers" ON)
if(SANTROLLER_TESTS_SANITIZE)
  add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=undefined)
  add_link_options(-fsanitize=address,undefined)
endif()

# Use an installed GoogleTest if there is one, otherwise fetch it
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/releases/download/v1.17.0/googletest-1.17.0.tar.gz
  URL_HASH SHA256=65fab701d9829d38cb77c14acdc431d2108bfdbf8979e40eb8ae567edf10b27c
  FIND_PACKAGE_ARGS NAMES GTest
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# The same nanopb protos the firmware is built with
include(${SANTROLLER_ROOT}/compile_proto.cmake)
compile_proto()
add_library(santroller_proto STATIC
  ${SANTROLLER_ROOT}/lib/nanopb/pb_common.c
  ${SANTROLLER_ROOT}/lib/nanopb/pb_encode.c
  ${SANTROLLER_ROOT}/lib/nanopb/pb_decode.c
  ${PROTO_OUTPUT_DIR}/input_enums.pb.c
  ${PROTO_OUTPUT_DIR}/enums.pb.c
  ${PROTO_OUTPUT_DIR}/device.pb.c
  ${PROTO_OUTPUT_DIR}/config.pb.c
  ${PROTO_OUTPUT_DIR}/input.pb.c
  ${PROTO_OUTPUT_DIR}/events.pb.c
  ${PROTO_OUTPUT_DIR}/commands.pb.c
)
target_include_directories(santroller_proto PUBLIC ${PROTO_OUTPUT_DIR} ${SANTROLLER_ROOT}/lib/nanopb)

# Firmware headers, plus TinyUSB's portable headers for the HID / MIDI definitions
add_library(santroller_headers INTERFACE)
target_include_directories(santroller_headers INTERFACE
  ${SANTROLLER_ROOT}/include
  ${SANTROLLER_ROOT}/lib/btstack/src
  ${SANTROLLER_ROOT}/lib/hidparser
  ${SANTROLLER_ROOT}/lib/tinyusb/src
)
target_link_libraries(santroller_headers INTERFACE santroller_proto)

add_library(santroller_hidparser STATIC ${SANTROLLER_ROOT}/lib/hidparser/hidparser.c)
target_link_libraries(santroller_hidparser PUBLIC santroller_headers)

add_executable(santroller_tests
  unit/hid_keyboard_test.cpp
  unit/ble_midi_test.cpp
  unit/hidparser_test.cpp
  unit/hid_mouse_test.cpp
  unit/midi_output_test.cpp
)
target_include_directories(santroller_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support)
target_compile_options(santroller_tests PRIVATE -Wall -Wextra -Wno-missing-field-initializers -Wno-unused-parameter)
target_link_libraries(santroller_tests PRIVATE santroller_hidparser GTest::gtest_main)

enable_testing()
include(GoogleTest)
gtest_discover_tests(santroller_tests DISCOVERY_MODE PRE_TEST)

# Each area of the firmware that needs its own sources or fakes keeps its test executable in cmake/<area>.cmake
set(SANTROLLER_TEST_ONLY_AREA "" CACHE STRING "Only include this area (e.g. midi), for working on one area at a time")
if(SANTROLLER_TEST_ONLY_AREA)
  set(SANTROLLER_TEST_AREAS ${CMAKE_CURRENT_SOURCE_DIR}/cmake/${SANTROLLER_TEST_ONLY_AREA}.cmake)
else()
  file(GLOB SANTROLLER_TEST_AREAS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/cmake/*.cmake)
endif()
foreach(area IN LISTS SANTROLLER_TEST_AREAS)
  include(${area})
endforeach()
