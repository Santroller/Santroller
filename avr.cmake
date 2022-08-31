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
              make OBJDIR=obj/${TARGET} VERSION_MAJOR=${VERSION_MAJOR} VERSION_MINOR=${VERSION_MINOR}
              VERSION_REVISION=${VERSION_REVISION} F_USB=${F_CPU} F_CPU=${F_CPU}
              ARDUINO_MODEL_PID=${PID} ARDWIINO_BOARD=${VARIANT} EXTRA=${EXTRA}
              TARGET=${OUT} MCU=${MCU} VARIANT=${${VARIANT}_VARIANT}
            WORKING_DIRECTORY ${IN}
            BYPRODUCTS ${IN}/obj/${TARGET} ${OUTPUTS})
        endforeach()
      endforeach()
    endforeach()
  endforeach()
endforeach()