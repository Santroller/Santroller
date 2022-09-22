set(PICO_BOARD ${BOARD})
pico_sdk_init()
set(TYPES "rf;main")
foreach(TYPE ${TYPES})
  unset(EXTRA)
  if(NOT (${TYPE} MATCHES "main"))
    set(EXTRA -${TYPE})
  endif()
  set(TARGET ardwiino-${BOARD}-rp2040${EXTRA})
  set(SRC src/pico/${TYPE})
  add_executable(
    ${TARGET}
    ${SRC}/main.c
    src/shared/controller/guitar_includes.c
    src/pico/lib/bootloader/bootloader.c
    src/shared/output/control_requests.c
    src/shared/output/descriptors.c
    src/shared/output/serial_handler.c
    src/shared/output/reports.c
    src/shared/leds/leds.c
    src/shared/rf/rf.c
    src/shared/input/input_handler.c
    src/pico/lib/eeprom/eeprom.c
    src/shared/lib/i2c/i2c_shared.c
    lib/avr-nrf24l01/src/nrf24l01.c
    lib/mpu6050/inv_mpu_dmp_motion_driver.c
    lib/mpu6050/inv_mpu.c
    lib/mpu6050/mpu_math.c
    lib/fxpt_math/fxpt_math.c
    src/pico/lib/util/util.c
    src/pico/lib/timer/timer.c
    src/pico/lib/spi/spi.c
    src/pico/lib/i2c/i2c.c
    src/pico/lib/usb/xinput_device.c
    src/pico/lib/pins/pins.c
    src/shared/lib/util/util_shared.c)
    target_include_directories(${TARGET} PUBLIC
      ${SRC}
      src/shared/output
      src/shared
      src/shared/lib
      lib
      src/pico
      src/pico/lib
      lib/lufa)
  if(${EXTRA} MATCHES "-rf")
    target_compile_definitions(${TARGET} PUBLIC RF_TX=true)
  endif()
  set(XIP_BASE 0x10000000)
  math(EXPR RF_TARGET_OFFSET "(256 * 1024)" OUTPUT_FORMAT HEXADECIMAL)
  math(EXPR FLASH_TARGET_OFFSET "(512 * 1024)" OUTPUT_FORMAT HEXADECIMAL)
  math(EXPR CONF_REGION "${XIP_BASE} + ${RF_TARGET_OFFSET}" OUTPUT_FORMAT HEXADECIMAL)
  math(EXPR RF_REGION "${XIP_BASE} + ${FLASH_TARGET_OFFSET}" OUTPUT_FORMAT HEXADECIMAL)
  # Define a memory section 256 kb away from the start of flash for the config
  target_link_options(${TARGET} PUBLIC -Wl,--section-start=.ardCfg=${CONF_REGION})
  # Define a memory section 512 kb away from the start of flash for the rf config
  target_link_options(${TARGET} PUBLIC -Wl,--section-start=.rfrecv=${RF_REGION})

  target_compile_definitions(
    ${TARGET}
    PUBLIC ARCH=3
           uint_reg_t=uint8_t
           PROGMEM=
           memcpy_P=memcpy
           strcpy_P=strcpy
           F_CPU=133000000
           PSTR=
           ARDWIINO_BOARD="${BOARD}"
           VERSION_MAJOR=${VERSION_MAJOR}
           PICO=1
           VERSION_MINOR=${VERSION_MINOR}
           VERSION_REVISION=${VERSION_REVISION}
           USE_INTERNAL_SERIAL=3
           FLASH_TARGET_OFFSET=${FLASH_TARGET_OFFSET}
           RF_TARGET_OFFSET=${RF_TARGET_OFFSET})

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
    tinyusb_host
    tinyusb_device
    tinyusb_board)

  # create map/bin/hex/uf2 file in addition to ELF.
  pico_add_extra_outputs(${TARGET})
  # Copy built binaries to firmware
  add_custom_command(
    TARGET ${TARGET}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.*
            ${CMAKE_CURRENT_BINARY_DIR}/firmware)
endforeach()

add_custom_target(pico)
add_dependencies(pico ardwiino-${BOARD}-rp2040)
add_custom_command(
  TARGET pico
  POST_BUILD
  COMMAND ${CMAKE_SOURCE_DIR}/scripts/bootloader.py)
add_custom_command(
  TARGET pico
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/firmware/ardwiino-${BOARD}-rp2040.uf2
    /run/media/$ENV{USER}/RPI-RP2/)