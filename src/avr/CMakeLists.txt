cmake_minimum_required(VERSION 3.13)
project(ardwiino C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
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
      if(NOT ${TYPE} MATCHES "usb")
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

      set(PID ${${VARIANT}_PID})
      foreach(MCU ${${VARIANT}_${BUILD_TYPE}_MCUS})
        foreach(F_CPU ${F_CPUS})
          if(${PROJECT} MATCHES "uno")
            set(TARGET
                ardwiino-${VARIANT}-${BUILD_TYPE}-${MCU}-${F_CPU}${EXTRA})
          else()
            set(TARGET ardwiino-${VARIANT}-${MCU}-${F_CPU}${EXTRA})
          endif()
          set(IN ${CMAKE_SOURCE_DIR}/src/avr)
          set(OUT ${CMAKE_CURRENT_BINARY_DIR}/firmware/${TARGET})
          add_custom_target(${TARGET} ALL)
          unset(OUTPUTS)
          foreach(OUTPUT ${OUTPUT_EXT})
            list(APPEND OUTPUTS ${OUT}.${OUTPUT})
          endforeach()
          add_custom_command(
            TARGET ${TARGET}
            COMMAND
              make VARIANT_ROOT=${CMAKE_SOURCE_DIR}/src/avr/${PROJECT} PROJECT_ROOT=${CMAKE_SOURCE_DIR} OBJDIR=${CMAKE_CURRENT_BINARY_DIR}/obj/${TARGET} F_USB=${F_CPU} F_CPU=${F_CPU}
              ARDUINO_MODEL_PID=${PID} ARDWIINO_BOARD=${VARIANT} EXTRA=${EXTRA}
              TARGET=${OUT} MCU=${MCU} VARIANT=${${VARIANT}_VARIANT}
            WORKING_DIRECTORY ${IN}
            BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/obj/${TARGET} ${OUTPUTS})
          message(STATUS ${IN})
          message(STATUS ${OUT})
        endforeach()
      endforeach()
    endforeach()
  endforeach()
endforeach()
