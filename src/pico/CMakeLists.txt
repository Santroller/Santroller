
set(PICO_SDK_PATH ${CMAKE_SOURCE_DIR}/submodules/pico-sdk)
set(PICO_EXTRAS_PATH ${CMAKE_SOURCE_DIR}/submodules/pico-extras)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
cmake_policy(SET CMP0057 NEW)
include(pico_sdk_import.cmake)
include(pico_extras_import.cmake)
pico_sdk_init()
project(ardwiino-pico C CXX ASM)
find_package(Git)
if(GIT_FOUND)
  message("git found: ${GIT_EXECUTABLE} in version ${GIT_VERSION_STRING}")
endif(GIT_FOUND)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/firmware)

set(TARGET ardwiino-pico-rp2040)
set(SRC .)
add_executable(
    ${TARGET}
    ../lib/descriptors.c
    ../lib/lib_main.c
    ../lib/report_descriptors.c
    ../lib/usb.c
    ../lib/i2c.c
    ../lib/leds.c
    pin.c
    timer.c
    main.c
    xinput_device.c
    usb_host/usb_host.c
)
target_include_directories(${TARGET} PUBLIC
      ${SRC}
      ../lib
      .)

set(XIP_BASE 0x10000000)
math(EXPR RF_TARGET_OFFSET "(256 * 1024)" OUTPUT_FORMAT HEXADECIMAL)
math(EXPR FLASH_TARGET_OFFSET "(512 * 1024)" OUTPUT_FORMAT HEXADECIMAL)
math(EXPR CONF_REGION "${XIP_BASE} + ${RF_TARGET_OFFSET}" OUTPUT_FORMAT HEXADECIMAL)
math(EXPR RF_REGION "${XIP_BASE} + ${FLASH_TARGET_OFFSET}" OUTPUT_FORMAT HEXADECIMAL)

# the pico doesn't really have the port concept that the avr controllers have
target_compile_definitions(${TARGET} PUBLIC
  PORTS=26
  NUM_DIGITAL_PINS=26
  NUM_ANALOG_INPUTS=3
  PINS_PER_PORT=1
  VERSION_MINOR=${VERSION_MINOR}
  VERSION_MAJOR=${VERSION_MAJOR}
  VERSION_REVISION=${VERSION_REVISION}
  FLASH_TARGET_OFFSET=${FLASH_TARGET_OFFSET}
  RF_TARGET_OFFSET=${RF_TARGET_OFFSET}
)
# Define a memory section 256 kb away from the start of flash for the config
target_link_options(${TARGET} PUBLIC -Wl,--section-start=.ardCfg=${CONF_REGION})
# Define a memory section 512 kb away from the start of flash for the rf config
target_link_options(${TARGET} PUBLIC -Wl,--section-start=.rfrecv=${RF_REGION})
if (CMAKE_BUILD_TYPE EQUAL "DEBUG")
  pico_enable_stdio_uart(${TARGET} 0)
  pico_enable_stdio_usb(${TARGET} 0)
else()
  pico_enable_stdio_uart(${TARGET} 1)
  pico_enable_stdio_usb(${TARGET} 0)
endif()
# Add pico_stdlib library which aggregates commonly used features
target_link_libraries(
  ${TARGET}
  pico_stdlib
  pico_multicore
  hardware_i2c
  hardware_dma
  hardware_spi
  hardware_adc
  hardware_gpio
  hardware_flash
  hardware_timer
  hardware_pio
  hardware_sleep
  pico_unique_id
  pico_mem_ops
  tinyusb_host
  tinyusb_device
  tinyusb_board)

pico_generate_pio_header(${TARGET} ${CMAKE_CURRENT_LIST_DIR}/usb_host/pio_serialiser.pio)
pico_generate_pio_header(${TARGET} ${CMAKE_CURRENT_LIST_DIR}/usb_host/pio_keepalive.pio)
pico_generate_pio_header(${TARGET} ${CMAKE_CURRENT_LIST_DIR}/usb_host/pio_bitstuff.pio)
# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(${TARGET})
# Copy built binaries to firmware
add_custom_command(
  TARGET ${TARGET}
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.*
          ${CMAKE_CURRENT_BINARY_DIR}/firmware)