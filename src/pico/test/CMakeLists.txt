cmake_minimum_required(VERSION 3.13)

include(../../../submodules/pico-sdk/pico_sdk_init.cmake)
include(../../../submodules/pico-extras/external/pico_extras_import.cmake)

project(pico_fs C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_C_COMPILER /bin/arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER /usr/bin/arm-none-eabi-g++)

# initialize the Pico SDK
pico_sdk_init()

include_directories(
  ../../../src/shared/output
  ../../../src/shared
  ../../../src/shared/lib
  ../../../lib
  ../../../src/pico
  ../../../src/pico/lib
  ../../../lib/lufa)
add_executable(pico_test_flash main.c 
../../../src/shared/controller/guitar_includes.c
../../../src/pico/lib/bootloader/bootloader.c
../../../src/shared/leds/leds.c
../../../src/shared/rf/rf.c
../../../src/shared/input/input_handler.c
../../../src/pico/lib/eeprom/eeprom.c
../../../src/shared/lib/i2c/i2c_shared.c
../../../lib/avr-nrf24l01/src/nrf24l01.c
../../../lib/mpu6050/inv_mpu_dmp_motion_driver.c
../../../lib/mpu6050/inv_mpu.c
../../../lib/mpu6050/mpu_math.c
../../../src/pico/lib/util/util.c
../../../src/pico/lib/timer/timer.c
../../../src/pico/lib/spi/spi.c
../../../src/pico/lib/i2c/i2c.c
../../../src/pico/lib/usb/xinput_device.c
../../../src/pico/lib/pins/pins.c)

set(XIP_BASE 0x10000000)
math(EXPR RF_TARGET_OFFSET "(256 * 1024)" OUTPUT_FORMAT HEXADECIMAL)
math(EXPR FLASH_TARGET_OFFSET "(512 * 1024)" OUTPUT_FORMAT HEXADECIMAL)
math(EXPR CONF_REGION "${XIP_BASE} + ${RF_TARGET_OFFSET}" OUTPUT_FORMAT HEXADECIMAL)
math(EXPR RF_REGION "${XIP_BASE} + ${FLASH_TARGET_OFFSET}" OUTPUT_FORMAT HEXADECIMAL)
target_link_libraries(pico_test_flash pico_stdlib hardware_flash)
target_compile_definitions(
  pico_test_flash
  PUBLIC ARCH=3
         uint_reg_t=uint8_t
         PROGMEM=
         memcpy_P=memcpy
         strcpy_P=strcpy
         F_CPU=133000000
         PSTR=
         ARDWIINO_BOARD="pico"
         VERSION_MAJOR=${VERSION_MAJOR}
         VERSION_MINOR=${VERSION_MINOR}
         VERSION_REVISION=${VERSION_REVISION}
         USE_INTERNAL_SERIAL=3
         FLASH_TARGET_OFFSET=${FLASH_TARGET_OFFSET}
         RF_TARGET_OFFSET=${RF_TARGET_OFFSET})
target_link_libraries(
    pico_test_flash
  pico_stdlib
  hardware_i2c
  hardware_spi
  hardware_adc
  hardware_gpio
  hardware_flash
  hardware_timer
  hardware_sleep
  pico_unique_id
  pico_mem_ops)
# enable usb output, disable uart output
pico_enable_stdio_usb(pico_test_flash 1)
pico_enable_stdio_uart(pico_test_flash 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(pico_test_flash)
add_custom_target(pico)
add_dependencies(pico pico_test_flash)
# add_custom_command(
#   TARGET pico
#   POST_BUILD
#   COMMAND ${CMAKE_SOURCE_DIR}/scripts/bootloader.py)
# add_custom_command(
#     TARGET pico
#     POST_BUILD
#     COMMAND sleep 10)
add_custom_command(
  TARGET pico
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/pico_test_flash.uf2
    /run/media/$ENV{USERNAME}/RPI-RP2/)