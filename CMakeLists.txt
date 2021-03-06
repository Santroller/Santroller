cmake_minimum_required(VERSION 3.13)
include(submodules/pico-sdk/pico_sdk_init.cmake)
include(submodules/pico-extras/external/pico_extras_import.cmake)
project(ardwiino C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_SYSTEM_PROCESSOR arm)
pico_sdk_init()
find_package(Git)
if(GIT_FOUND)
  message("git found: ${GIT_EXECUTABLE} in version ${GIT_VERSION_STRING}")
endif(GIT_FOUND)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/firmware)
# For AVR, we have projects for arduinos that are similar enough to run the same
# code after that there are variants that specify small differences, such as the
# micro vs leo
set(PROJECTS "micro;uno")
set(OUTPUT_EXT "bin;eep;elf;hex;lss;map;sym")

# for usbserial firmwares we need the original arduino pids
set(uno_PID 0x0001)
set(mega2560_PID 0x0010)
set(megaadk_PID 0x003f)

# List all processors used by different variants
set(uno_main_MCUS atmega328p)
set(mega2560_main_MCUS atmega2560)
set(megaadk_main_MCUS atmega2560)
set(mini_main_MCUS atmega328p)

set(uno_usb_MCUS "atmega16u2;at90usb82")
set(mega2560_usb_MCUS "atmega16u2;at90usb82")
set(megaadk_usb_MCUS "atmega16u2;at90usb82")

set(micro_main_MCUS atmega32u4)
set(a-micro_main_MCUS atmega32u4)
set(leonardo_main_MCUS atmega32u4)

# map the variants to their respective header files in src/avr/variants
set(micro_VARIANT micro)
set(a-micro_VARIANT micro)
set(leonardo_VARIANT micro)

set(mini_VARIANT uno)
set(uno_VARIANT uno)

set(mega2560_VARIANT mega)
set(megaadk_VARIANT mega)

# for micro based builds, the multi firmare is based on the main firmware, but
# it is based on the usb firmware for unos.
set(uno_multi_BUILD_TYPE usb)
set(micro_multi_BUILD_TYPE main)

set(uno_VARIANTS "uno;mega2560;megaadk;mini")
set(micro_VARIANTS "micro;a-micro;leonardo")
set(F_CPU_8_mini TRUE)
set(F_CPU_8_micro TRUE)
foreach(PROJECT ${PROJECTS})
  foreach(VARIANT ${${PROJECT}_VARIANTS})
    set(TYPES "rf;multi;main")
    if(${PROJECT} MATCHES "uno")
      list(APPEND TYPES usb)
      list(APPEND TYPES usbserial)
    endif()
    # Minis only support RF, as they don't have usb
    if(${VARIANT} MATCHES "mini")
      set(TYPES rf)
    endif()
    foreach(TYPE ${TYPES})
      set(F_CPUS 16000000)
      # If 8mhz is supported then add it to the list of built frequencies
      if((${F_CPU_8_${PROJECT}}) OR (${F_CPU_8_${VARIANT}}))
        list(APPEND F_CPUS 8000000)
      endif()
      # Find the correct type of build for the spefied type
      if(${TYPE} MATCHES "multi")
        set(BUILD_TYPE ${${PROJECT}_multi_BUILD_TYPE})
      elseif(NOT ${TYPE} MATCHES "usb")
        set(BUILD_TYPE main)
      else()
        set(BUILD_TYPE usb)
      endif()
      # Handle appending things to the end for special builds
      unset(EXTRA)
      set(DIR ${BUILD_TYPE})
      if(NOT (${TYPE} MATCHES "(usb|main)$"))
        set(EXTRA -${TYPE})
      endif()
      # usbserial and rf builds have their own directories, They aren't just
      # flags like multi
      if(NOT ${TYPE} MATCHES "usbserial|rf")
        set(TYPE ${BUILD_TYPE})
      endif()

      set(PID ${${VARIANT}_PID})
      foreach(MCU ${${VARIANT}_${BUILD_TYPE}_MCUS})
        foreach(F_CPU ${F_CPUS})
          if(${PROJECT} MATCHES "uno")
            set(TARGET
                ardwiino-${VARIANT}-${BUILD_TYPE}-${MCU}-${F_CPU}${EXTRA})
          else()
            set(TARGET ardwiino-${VARIANT}-${MCU}-${F_CPU}${EXTRA})
          endif()
          set(IN ../src/avr/${PROJECT}/${TYPE})
          set(OUT ${CMAKE_CURRENT_BINARY_DIR}/firmware/${TARGET})
          add_custom_target(${TARGET} ALL)
          unset(OUTPUTS)
          foreach(OUTPUT ${OUTPUT_EXT})
            list(APPEND OUTPUTS ${OUT}.${OUTPUT})
          endforeach()
          add_custom_command(
            TARGET ${TARGET}
            COMMAND
              make OBJDIR=obj/${TARGET} F_USB=${F_CPU} F_CPU=${F_CPU}
              ARDUINO_MODEL_PID=${PID} ARDWIINO_BOARD=${VARIANT} EXTRA=${EXTRA}
              TARGET=${OUT} MCU=${MCU} VARIANT=${${VARIANT}_VARIANT}
            WORKING_DIRECTORY ${IN}
            BYPRODUCTS ${IN}/obj/${TARGET} ${OUTPUTS})
          message(STATUS ${IN})
          message(STATUS ${OUT})
        endforeach()
      endforeach()
    endforeach()
  endforeach()
endforeach()
set(TYPES "rf;multi;main")
foreach(TYPE ${TYPES})
  unset(EXTRA)
  if(NOT (${TYPE} MATCHES "main"))
    set(EXTRA -${TYPE})
  endif()
  set(TARGET ardwiino-pico-rp2040${EXTRA})
  if(${TYPE} MATCHES "multi")
    set(TYPE main)
  endif()
  set(SRC src/pico/${TYPE})
  message(STATUS ${TARGET})
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
    lib/avr-nrf24l01/src/nrf24l01.c
    lib/mpu6050/inv_mpu_dmp_motion_driver.c
    lib/mpu6050/inv_mpu.c
    lib/mpu6050/mpu_math.c
    src/pico/lib/util/util.c
    src/pico/lib/timer/timer.c
    src/pico/lib/spi/spi.c
    src/pico/lib/i2c/i2c.c
    src/pico/lib/usb/xinput_device.c
    src/pico/lib/pins/pins.c)
  include_directories(
    src/shared/output
    src/shared
    src/shared/lib
    lib
    src/pico
    src/pico/lib
    lib/lufa)
  if(${EXTRA} MATCHES "-multi")
    target_compile_definitions(${TARGET} PUBLIC MULTI_ADAPTOR)
  endif()
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

  if(${TYPE} MATCHES "main")
    include_directories(src/pico/main)
  endif()

  execute_process(COMMAND ${GIT_EXECUTABLE} --git-dir=${CMAKE_SOURCE_DIR}/.git
                          describe --abbrev=0 --tags OUTPUT_VARIABLE OUTPUT)
  string(REPLACE version- "" OUTPUT ${OUTPUT})
  string(REPLACE "." ";" OUTPUT ${OUTPUT})
  list(GET OUTPUT 0 VERSION_MAJOR)
  list(GET OUTPUT 1 VERSION_MINOR)
  list(GET OUTPUT 2 VERSION_REVISION)
  string(STRIP ${VERSION_REVISION} VERSION_REVISION)

  target_compile_definitions(
    ${TARGET}
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

  # Add pico_stdlib library which aggregates commonly used features
  target_link_libraries(
    ${TARGET}
    pico_stdlib
    hardware_i2c
    hardware_spi
    hardware_adc
    hardware_gpio
    hardware_flash
    hardware_timer
    hardware_sleep
    pico_unique_id
    pico_mem_ops
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
