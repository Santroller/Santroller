cmake_minimum_required(VERSION 3.13)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(PICO_EXTRAS_FETCH_FROM_GIT TRUE)
set(PICO_SDK_FETCH_FROM_GIT TRUE)
set(PICO_PIO_USB_PATH ${CMAKE_SOURCE_DIR}/lib/pico_pio_usb)
set(PICO_TINYUSB_PATH ${CMAKE_SOURCE_DIR}/lib/tinyusb)
set(PICO_BTSTACK_PATH ${CMAKE_SOURCE_DIR}/lib/btstack)
set(PICO_BOARD "pico_w")
set(BLUEPAD32_ROOT ${CMAKE_SOURCE_DIR}/lib/bluepad32)

include(pico_sdk_import.cmake)
include(pico_extras_import.cmake)

project(santroller C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

add_compile_options(-Wall
  -Wtype-limits
  -Wno-format # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
  -Wno-unused-function # we have some for the docs that aren't called
)

include(compile_proto.cmake)
compile_proto()

pico_sdk_init()

add_compile_options(-fdata-sections -ffunction-sections)
add_link_options(-Wl,--gc-sections)

add_subdirectory(lib)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  # Activate some compiler / linker options to aid us with diagnosing stack space issues in Debug builds
  add_compile_options(-fstack-usage -Wstack-usage=500)
  add_compile_definitions(PICO_USE_STACK_GUARDS=1)
endif()

# We want a larger stack of 4kb per core instead of the default 2kb
add_compile_definitions(PICO_STACK_SIZE=0x1000)

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
  src/main.cpp
  ${PROTO_OUTPUT_DIR}/enums.pb.c
  ${PROTO_OUTPUT_DIR}/peripheral.pb.c
  ${PROTO_OUTPUT_DIR}/config.pb.c
)
target_include_directories(${TARGET} PUBLIC
  ${SRC}
  include
  ${PROTO_OUTPUT_DIR}
  ${CMAKE_BINARY_DIR}/headers)

# if (CMAKE_BUILD_TYPE EQUAL "DEBUG")
# pico_enable_stdio_uart(${TARGET} 0)
# pico_enable_stdio_usb(${TARGET} 0)
# else()
# pico_enable_stdio_uart(${TARGET} 1)
# pico_enable_stdio_usb(${TARGET} 0)

# endif()
pico_set_program_name(${TARGET} "Santroller")
pico_set_program_version(${TARGET} ${GIT_REPO_VERSION})

target_link_libraries(
  ${TARGET}
  pico_stdlib
  midi
  peripherals
  accelerometer
  band_hero_drum
  crazy_guitar_neck
  dj_hero_turntable
  guitar_hero_5_neck
  xgip_protocol
  matrix
  max1704x
  mpr121
  multiplexer
  psx_controller
  secondary_pico
  wii_extensions
  world_tour_drum
  psx_emulation
  wii_extension_emulation
  wii_remote_emulation
  SNESpad
  n64_gamecube_controller
  ws2812
  nanopb
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