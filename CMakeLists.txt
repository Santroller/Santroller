cmake_minimum_required(VERSION 3.6)
project(foo)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/firmware)
# TODO there is defo a cleaner way to implement this
list (APPEND VARIANTS micro)
list (APPEND VARIANTS a-micro)
list (APPEND VARIANTS leonardo)
list (APPEND MICRO_F_CPU 8000000)
list (APPEND MICRO_F_CPU 16000000)
list (APPEND MULTI -multi)
list (APPEND MULTI -n)
list (APPEND OUTPUT_EXT bin)
list (APPEND OUTPUT_EXT eep)
list (APPEND OUTPUT_EXT elf)
list (APPEND OUTPUT_EXT hex)
list (APPEND OUTPUT_EXT lss)
list (APPEND OUTPUT_EXT map)
list (APPEND OUTPUT_EXT sym)
foreach (MUL ${MULTI})
    unset(DIRS)
    list (APPEND DIRS main)
    if(${MUL} MATCHES "-n") 
        unset(MUL)
        list (APPEND DIRS rf)
    endif()
    foreach (DIR ${DIRS})
        foreach (VARIANT ${VARIANTS})
            foreach (F_CPU ${MICRO_F_CPU})
                unset(DIR2)
                if(${DIR} MATCHES "rf") 
                    set(DIR2 -rf)
                endif()
                add_custom_target(${DIR}-${VARIANT}-${F_CPU}${MUL}${DIR2} ALL)
                unset(OUTPUTS)
                foreach(OUTPUT ${OUTPUT_EXT}) 
                    list(APPEND OUTPUTS ${CMAKE_CURRENT_BINARY_DIR}/firmware/ardwiino-${VARIANT}-atmega32u4-${F_CPU}${DIR2}.${OUTPUT})
                endforeach()
                add_custom_command(
                    TARGET   ${DIR}-${VARIANT}-${F_CPU}${MUL}${DIR2}
                    COMMAND  make OBJDIR=obj/${DIR}-${VARIANT}-${F_CPU}${MUL} F_USB=${F_CPU} F_CPU=${F_CPU} ARDWIINO_BOARD=${VARIANT} MULTI=${MUL} TARGET=${CMAKE_CURRENT_BINARY_DIR}/firmware/ardwiino-${VARIANT}-atmega32u4-${F_CPU}${DIR2}
                    WORKING_DIRECTORY ../src/avr/micro/${DIR}
                    BYPRODUCTS ../src/avr/micro/${DIR}/obj/${DIR}-${VARIANT}-${F_CPU}${MUL} ${OUTPUTS}
                )
            endforeach()
        endforeach()
    endforeach()
endforeach()
set (uno_PID 0x0001)
set (mega2560_PID 0x0001)
set (megaadk_PID 0x0001)
set (uno_MCU atmega328p)
set (mega2560_MCU mega2560)
set (megaadk_MCU mega2560)
set (uno_VARIANT uno)
set (mega2560_VARIANT mega)
set (megaadk_VARIANT mega)

list (APPEND UNO_VARIANTS uno)
list (APPEND UNO_VARIANTS mega2560)
list (APPEND UNO_VARIANTS megaadk)
list (APPEND UNO_MCUS atmega16u2)
list (APPEND UNO_MCUS at90usb82)

foreach (MUL ${MULTI})
    unset(DIRS)
    list (APPEND DIRS usb)
    if(${MUL} MATCHES "-n") 
        unset(MUL)
        list (APPEND DIRS rf)
        list (APPEND DIRS main)
    endif()
    foreach (DIR ${DIRS})
        foreach (VARIANT ${UNO_VARIANTS})
            unset(DIR2)
            set(DIRM ${DIR})
            if(${DIR} MATCHES "rf") 
                set(DIR2 -rf)
                set(DIRM main)
            endif()
            unset(MCUS)
            if(${DIR} MATCHES "usb") 
                set(MCUS ${UNO_MCUS})
            else()
                set(MCUS ${${VARIANT}_MCU})
            endif()
            
            foreach (MCU ${MCUS})
                if(${MUL} MATCHES "-multi") 
                    set(PID ${${VARIANT}_PID})
                    unset(OUTPUTS)
                    foreach(OUTPUT ${OUTPUT_EXT}) 
                        list(APPEND OUTPUTS ${CMAKE_CURRENT_BINARY_DIR}/firmware/ardwiino-${VARIANT}-${DIRM}-${MCU}-16000000-usbserial.${OUTPUT})
                    endforeach()
                    add_custom_target(${DIR}-${VARIANT}-${MCU}-usbserial ALL)

                    add_custom_command(
                        TARGET   ${DIR}-${VARIANT}-${MCU}-usbserial
                        COMMAND  make OBJDIR=obj/${DIR}-${VARIANT} ARDUINO_MODEL_PID=${PID} TARGET=${CMAKE_CURRENT_BINARY_DIR}/firmware/ardwiino-${VARIANT}-${DIRM}-${MCU}-16000000-usbserial
                        WORKING_DIRECTORY ../src/avr/uno/original-usb
                        BYPRODUCTS ../src/avr/uno/original-usb/obj/${DIR}-${VARIANT} ${OUTPUTS}
                    )
                endif()

                unset(OUTPUTS)
                foreach(OUTPUT ${OUTPUT_EXT}) 
                    list(APPEND OUTPUTS ${CMAKE_CURRENT_BINARY_DIR}/firmware/ardwiino-${VARIANT}-${DIRM}-${MCU}-16000000${DIR2}.${OUTPUT})
                endforeach()
                add_custom_target(${VARIANT}${MUL}${DIR2}${MCU} ALL)

                add_custom_command(
                    TARGET   ${VARIANT}${MUL}${DIR2}${MCU}
                    COMMAND  make OBJDIR=obj/${DIR}-${VARIANT}${MUL}${DIR2}${MCU} ARDWIINO_BOARD=${VARIANT} MULTI=${MUL} TARGET=${CMAKE_CURRENT_BINARY_DIR}/firmware/ardwiino-${VARIANT}-${DIRM}-${MCU}-16000000${DIR2}
                    WORKING_DIRECTORY ../src/avr/uno/${DIR}
                    BYPRODUCTS ../src/avr/uno/${DIR}/obj/${DIR}-${VARIANT}${MUL}${DIR2}${MCU} ${OUTPUTS}
                )
            endforeach()
        endforeach()
    endforeach()
endforeach()
foreach (F_CPU ${MICRO_F_CPU})

    unset(OUTPUTS)
    foreach(OUTPUT ${OUTPUT_EXT}) 
        list(APPEND OUTPUTS ${CMAKE_CURRENT_BINARY_DIR}/firmware/ardwiino-mini-main-atmega328p-${F_CPU}-rf.${OUTPUT})
    endforeach()
    add_custom_target(mini-${F_CPU} ALL)

    add_custom_command(
        TARGET   mini-${F_CPU}
        COMMAND  make OBJDIR=obj/mini-${F_CPU} F_USB=${F_CPU} F_CPU=${F_CPU} MCU=atmega328p ARDWIINO_BOARD=mini VARIANT=uno TARGET=${CMAKE_CURRENT_BINARY_DIR}/firmware/ardwiino-mini-main-atmega328p-${F_CPU}-rf
        WORKING_DIRECTORY ../src/avr/mini/rf/${DIR}
        BYPRODUCTS ../src/avr/mini/rf/obj/mini-${F_CPU} ${OUTPUTS}
    )
endforeach()
set_directory_properties(PROPERTIES ${CMAKE_CURRENT_BINARY_DIR}/firmware ADDITIONAL_MAKE_CLEAN_FILES)