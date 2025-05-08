cmake_minimum_required(VERSION 3.13)
set(PICO_EXTRAS_FETCH_FROM_GIT TRUE)
set(PICO_SDK_FETCH_FROM_GIT TRUE)
set(PICO_PIO_USB_PATH ${CMAKE_SOURCE_DIR}/lib/pico_pio_usb)
set(PICO_TINYUSB_PATH ${CMAKE_SOURCE_DIR}/lib/tinyusb)
set(PICO_BTSTACK_PATH ${CMAKE_SOURCE_DIR}/lib/btstack)

include(pico_sdk_import.cmake)
include(pico_extras_import.cmake)

project(santroller C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

find_package(Git)
execute_process(COMMAND ${GIT_EXECUTABLE} describe --tags --always --dirty --abbrev=7
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_REPO_VERSION
  OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${GIT_EXECUTABLE} describe --always --abbrev=7
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_REPO_BUILD_ID
  OUTPUT_STRIP_TRAILING_WHITESPACE)
string(REGEX REPLACE "v([0-9]+\\.[0-9]+\\.[0-9]+).*" "\\1" CMAKE_GIT_REPO_VERSION ${GIT_REPO_VERSION})
string(REGEX REPLACE "^(.......-.*)|(.......)$" "0.0.0" CMAKE_GIT_REPO_VERSION ${CMAKE_GIT_REPO_VERSION}) # fix if all we have is the git SHA
configure_file("include/version.h.in" "include/version.h")
message("GIT_REPO_VERSION is ${GIT_REPO_VERSION}")
message("CMAKE_GIT_REPO_VERSION is ${CMAKE_GIT_REPO_VERSION}")
message("GIT_REPO_BUILD_ID is ${GIT_REPO_BUILD_ID}")

set(TARGET santroller)
message(STATUS ${TARGET})
add_executable(
  ${TARGET}
  src/hal/pico/main.cpp
  src/hal/pico/xinput_host.cpp
  lib/midi/MIDI.cpp
)
target_include_directories(${TARGET} PUBLIC
  ${SRC}
  lib/midi
  include
  include/hal/pico)

# if (CMAKE_BUILD_TYPE EQUAL "DEBUG")
# pico_enable_stdio_uart(${TARGET} 0)
# pico_enable_stdio_usb(${TARGET} 0)
# else()
pico_enable_stdio_uart(${TARGET} 1)
pico_enable_stdio_usb(${TARGET} 0)

# endif()
# pico_generate_pio_header(${TARGET} src/pico/lib/spi/spi.pio)
# Add pico_stdlib library which aggregates commonly used features
target_link_libraries(
  ${TARGET}
  pico_stdlib
  hardware_i2c
  hardware_spi
  hardware_adc
  hardware_pio
  hardware_gpio
  hardware_flash
  hardware_timer
  hardware_sleep
  pico_unique_id
  pico_mem_ops
  pico_bootsel_via_double_reset
  tinyusb_host
  tinyusb_pico_pio_usb
  tinyusb_device_unmarked)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(${TARGET})

add_custom_target(pico)
add_dependencies(pico santroller)