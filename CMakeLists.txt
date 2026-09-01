# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.3.0)
set(toolchainVersion 15_2_Rel1)
set(picotoolVersion 2.3.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
cmake_minimum_required(VERSION 3.13)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(SANTROLLER_BOARD_VARIANTS pico_w pico2_w CACHE STRING "Board variants to build")
option(SANTROLLER_VARIANT_DRIVER "Configure this build tree as a board variant driver" OFF)
option(SANTROLLER_VARIANT_BUILD "Internal flag for single-board variant builds" OFF)
option(ENABLE_WIFI "Enable WiFi support (CYW43 driver)" ON)
set(ENABLE_XBOX_WIRELESS ON CACHE BOOL "Enable Xbox Wireless Adapter support")

if(SANTROLLER_VARIANT_DRIVER AND NOT SANTROLLER_VARIANT_BUILD)
  project(santroller_variants NONE)

  add_custom_target(prepare_board_variants
    COMMAND ${CMAKE_COMMAND} -E rm -f "${CMAKE_BINARY_DIR}/santroller.uf2" "${CMAKE_BINARY_DIR}/santroller_fota.uf2"
  )

  foreach(SANTROLLER_VARIANT IN LISTS SANTROLLER_BOARD_VARIANTS)
    set(SANTROLLER_VARIANT_BINARY_DIR "${CMAKE_BINARY_DIR}/${SANTROLLER_VARIANT}")
    add_custom_target(build_${SANTROLLER_VARIANT} ALL
      COMMAND ${CMAKE_COMMAND}
        -Wno-dev
        -S "${CMAKE_SOURCE_DIR}"
        -B "${SANTROLLER_VARIANT_BINARY_DIR}"
        -G "${CMAKE_GENERATOR}"
        -DPICO_BOARD=${SANTROLLER_VARIANT}
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DCMAKE_EXPORT_COMPILE_COMMANDS=${CMAKE_EXPORT_COMPILE_COMMANDS}
        -DENABLE_WIFI=${ENABLE_WIFI}
        -DENABLE_XBOX_WIRELESS=${ENABLE_XBOX_WIRELESS}
        -DSANTROLLER_VARIANT_BUILD=ON
      COMMAND ${CMAKE_COMMAND} --build "${SANTROLLER_VARIANT_BINARY_DIR}" --parallel
      COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SANTROLLER_VARIANT_BINARY_DIR}/santroller.uf2" "${CMAKE_BINARY_DIR}/santroller_${SANTROLLER_VARIANT}.uf2"
      COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SANTROLLER_VARIANT_BINARY_DIR}/santroller_fota.uf2" "${CMAKE_BINARY_DIR}/santroller_${SANTROLLER_VARIANT}_fota.uf2"
    )
    add_dependencies(build_${SANTROLLER_VARIANT} prepare_board_variants)
  endforeach()

  return()
endif()

set(PICO_EXTRAS_FETCH_FROM_GIT TRUE)
set(PICO_SDK_FETCH_FROM_GIT TRUE)
set(PICO_PIO_USB_PATH ${CMAKE_SOURCE_DIR}/lib/pico_pio_usb)
set(PICO_TINYUSB_PATH ${CMAKE_SOURCE_DIR}/lib/tinyusb)
set(PICO_BTSTACK_PATH ${CMAKE_SOURCE_DIR}/lib/btstack)
set(PICO_BOARD pico_w CACHE STRING "Board type")
set(BLUEPAD32_ROOT ${CMAKE_SOURCE_DIR}/lib/bluepad32)
set(PFB_WITH_IMAGE_ENCRYPTION OFF)
set(PFB_WITH_SHA256_HASHING OFF)
set(PFB_RESERVED_FILESYSTEM_SIZE_KB 8)
set(PFB_WITH_BOOTLOADER_LOGS OFF)

include(pico_sdk_import.cmake)
include(pico_extras_import.cmake)

project(santroller C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
# set(PFB_REDIRECT_BOOTLOADER_LOGS_TO_UART ON)
add_compile_options(-Wall
  -Wtype-limits
  -Wno-format # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
  -Wno-unused-function # we have some for the docs that aren't called
  -Werror=return-type
)

include(compile_proto.cmake)
compile_proto()

pico_sdk_init()

add_compile_options(-fdata-sections -ffunction-sections)
add_link_options(-Wl,--gc-sections)

include(cmake/Uzlib.cmake)
add_subdirectory(lib)
add_subdirectory(lib/pico_fota_bootloader)


if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  # Activate some compiler / linker options to aid us with diagnosing stack space issues in Debug builds
  add_compile_options(-fstack-usage -Wstack-usage=500)
  add_compile_definitions(PICO_USE_STACK_GUARDS=1)
  set_source_files_properties(
    ${PICO_SDK_PATH}/lib/mbedtls/library/aes.c
    ${PICO_SDK_PATH}/lib/mbedtls/library/ctr_drbg.c
    PROPERTIES COMPILE_OPTIONS -Wno-stack-usage
  )
  # add_compile_definitions(PICO_DEBUG_MALLOC=1)
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
execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --short=8 HEAD
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_COMMIT
  OUTPUT_STRIP_TRAILING_WHITESPACE)
string(REGEX REPLACE "v([0-9]+\\.[0-9]+\\.[0-9]+).*" "\\1" CMAKE_GIT_REPO_VERSION ${GIT_REPO_VERSION})
string(REGEX REPLACE "^(.......-.*)|(.......)$" "0.0.0" CMAKE_GIT_REPO_VERSION ${CMAKE_GIT_REPO_VERSION}) # fix if all we have is the git SHA
configure_file("include/version.h.in" "include/version.h")
message("GIT_REPO_VERSION is ${GIT_REPO_VERSION}")
message("CMAKE_GIT_REPO_VERSION is ${CMAKE_GIT_REPO_VERSION}")
message("GIT_REPO_BUILD_ID is ${GIT_REPO_BUILD_ID}")
add_compile_definitions("GIT_HASH=\"${GIT_COMMIT}\"")

set(TARGET santroller)
message(STATUS ${TARGET})
FILE(WRITE ${CMAKE_BINARY_DIR}/commit.hash.in
"${GIT_COMMIT}"
)
add_executable(
  ${TARGET}
  src/main.cpp
  src/emulation/usb/usb_descriptors.cpp
  src/emulation/usb/xinput_device.cpp
  src/emulation/usb/xone_device.cpp
  src/emulation/usb/ogxbox_device.cpp
  src/emulation/usb/ps3_device.cpp
  src/emulation/usb/ps4_device.cpp
  src/emulation/usb/ps5_device.cpp
  src/emulation/usb/hid_device.cpp
  src/emulation/usb/hid_keyboard_device.cpp
  src/emulation/usb/hid_gamepad_device.cpp
  src/emulation/usb/hid_config_device.cpp
  src/emulation/usb/switch_device.cpp
  src/emulation/usb/gh_arcade_device.cpp
  src/devices/usb/host/xone_host.cpp
  src/devices/usb/host/midi_host.cpp
  src/devices/usb/host/ogxbox_host.cpp
  src/devices/usb/host/hid/hid_host.cpp
  src/devices/usb/host/hid/ps3_host.cpp
  src/devices/usb/host/hid/ps4_host.cpp
  src/devices/usb/host/hid/ps5_host.cpp
  src/devices/usb/host/hid/mouse_host.cpp
  src/devices/usb/host/hid/keyboard_host.cpp
  src/devices/usb/host/hid/generic_host.cpp
  src/devices/usb/host/hid/ltek_host.cpp
  src/devices/usb/host/hid/raphnet_host.cpp
  src/devices/usb/host/hid/stadia_host.cpp
  src/devices/usb/host/hid/stepmania_host.cpp
  src/devices/usb/host/hid/streamdeck_host.cpp
  src/devices/usb/host/hid/switch_host.cpp
  src/devices/usb/host/xinput/xinput_audio_host.cpp
  src/devices/usb/host/xinput/xinput_gamepad_host.cpp
  src/devices/usb/host/xinput/xinput_wireless_gamepad_host.cpp
  src/devices/usb/host/xinput/xinput_wireless_audio_host.cpp
  src/devices/usb/host/xinput/xinput_big_button_host.cpp
  src/devices/usb/host/xinput/xinput_plugin_module_host.cpp
  src/devices/usb/host/xinput/xinput_security_host.cpp
  src/emulation/ps2_emulation.cpp
  src/emulation/wii_emulation.cpp
  src/input/accelerometer.cpp
  src/input/crkd.cpp
  src/input/crkd_drum.cpp
  src/input/gh5.cpp
  src/input/gpio.cpp
  src/input/matrix.cpp
  src/input/wii.cpp
  src/input/shortcut.cpp
  src/input/usb.cpp
  src/input/ps2.cpp
  src/input/midi.cpp
  src/input/mpr121.cpp
  src/input/protarNeck.cpp
  src/input/vtechexpander.cpp
  src/input/cycle.cpp
  src/input/toggle.cpp
  src/input/held.cpp
  src/input/fixed.cpp
  src/input/ads1115.cpp
  src/input/multiplexer.cpp
  src/leds/led_devices.cpp
  src/leds/led_mappings.cpp
  src/triggers/activation_trigger_list.cpp
  src/triggers/input_trigger.cpp
  src/triggers/device_type_triggers.cpp
  src/triggers/mode_triggers.cpp
  src/managers/profile_manager.cpp
  src/managers/device_manager.cpp
  src/managers/config_manager.cpp
  src/profiles/profile.cpp
  src/config/device_factory.cpp
  src/config/input_factory.cpp
  src/config/mapping_factory.cpp
  src/config/trigger_factory.cpp
  src/config/led_factory.cpp
  src/config/instance_factory.cpp
  src/mappings/base_mapping.cpp
  src/mappings/gamepad_mapping.cpp
  src/mappings/red_octane_mappings.cpp
  src/mappings/rock_band_mappings.cpp
  src/mappings/konami_mappings.cpp
  src/mappings/djmax_mappings.cpp
  src/mappings/project_diva_mappings.cpp
  src/mappings/taiko_mappings.cpp
  src/mappings/peripheral_mappings.cpp
  src/mappings/powergig_mappings.cpp
  src/mappings/rock_revolution_mappings.cpp
  src/devices/bt/ble_rx.cpp
  src/devices/bt/bt_classic_rx.cpp
  src/devices/usb.cpp
  src/usb/auth_broker.cpp
  src/devices/wii.cpp
  src/devices/ps2.cpp
  src/devices/ps2_emulation.cpp
  src/devices/wii_emulation.cpp
  src/devices/crkd.cpp
  src/devices/crkd_drum.cpp
  src/devices/ads1115.cpp
  src/devices/device.cpp
  src/devices/accelerometer.cpp
  src/devices/bhdrum.cpp
  src/devices/wtdrum.cpp
  src/devices/protar_neck.cpp
  src/devices/debug.cpp
  src/devices/matrix.cpp
  src/devices/vtechexpander.cpp
  src/devices/cycle.cpp
  src/devices/toggle.cpp
  src/devices/crazyneck.cpp
  ${CYW43_FIRMWARE_SOURCES}
  src/devices/multiplexer.cpp
  src/devices/midiserial.cpp
  src/devices/djh.cpp
  src/devices/midi.cpp
  src/devices/gh5neck.cpp
  src/devices/max1704x.cpp
  src/devices/mpr121.cpp
  src/devices/ws2812.cpp
  src/devices/apa102.cpp
  src/devices/dmx.cpp
  src/devices/led.cpp
  src/devices/bluetooth.cpp
  src/devices/stp16cpc.cpp
  src/config/config.cpp
  src/config/config_loader.cpp
  src/config/config_storage.cpp
  src/emulation/bt/bt_gamepad.cpp
  src/emulation/bt/bt_descriptors.cpp
  ${PROTO_OUTPUT_DIR}/input_enums.pb.c
  ${PROTO_OUTPUT_DIR}/enums.pb.c
  ${PROTO_OUTPUT_DIR}/device.pb.c
  ${PROTO_OUTPUT_DIR}/config.pb.c
  ${PROTO_OUTPUT_DIR}/input.pb.c
  ${PROTO_OUTPUT_DIR}/events.pb.c
  ${PROTO_OUTPUT_DIR}/commands.pb.c
)

# Xbox Wireless Adapter (MT76) Support
if(ENABLE_XBOX_WIRELESS)
    message(STATUS "Xbox Wireless Adapter support enabled")
    
    # Use Pico SDK's mbedTLS with custom config for Xbox Wireless crypto
    set(HAVE_MBEDTLS TRUE)
    message(STATUS "Using Pico SDK's mbedTLS with Xbox Wireless crypto features")

    # Download and convert firmware
    include(cmake/DownloadFirmware.cmake)
    include(cmake/ConvertFirmware.cmake)

    # Add MT76 sources to main executable
    target_sources(${TARGET} PRIVATE
        src/devices/usb/host/xbox/mt76.c
        src/devices/usb/host/xbox/mt76_init.c
        src/devices/usb/host/xbox/wireless.c
        src/devices/usb/host/xbox/gip_crypto.c
        src/devices/usb/host/xbox/firmware_decompress.c
        src/devices/usb/host/xbox_wireless_host.cpp
        src/devices/usb/host/xbox_wireless_controller.cpp
        ${CMAKE_BINARY_DIR}/generated/firmware_data.c
    )

    # Add include directories
    target_include_directories(${TARGET} PRIVATE
        include/devices/usb/host/xbox
        ${CMAKE_BINARY_DIR}/generated
    )

    # Link uzlib for firmware decompression
    target_link_libraries(${TARGET} uzlib)
    
    # Link Pico SDK's mbedTLS libraries
    if(HAVE_MBEDTLS)
        target_compile_definitions(${TARGET} PRIVATE HAVE_MBEDTLS=1)
        target_link_libraries(${TARGET}
            pico_mbedtls
        )
        message(STATUS "mbedTLS libraries linked to ${TARGET}")
    endif()
    
    message(STATUS "Xbox Wireless Adapter integration complete")
endif()

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
  pico_rand
  pico_btstack_cyw43
  pico_btstack_classic
  pico_btstack_ble
  pico_cyw43_arch_threadsafe_background
  FlashPROM
  ring_buffer
  CRC32
  protar_neck
  crkd_neck
  crkd_drum
  wm_crypto
  peripherals
  quadrature_encoder
  accelerometer
  band_hero_drum
  midi_serial
  crazy_guitar_neck
  dj_hero_turntable
  guitar_hero_5_neck
  gip_common
  xgip_protocol
  vtech_guitar_ioexpander
  pico_audio_i2s
  xsm3
  matrix
  max1704x
  mpr121
  ads1115
  multiplexer
  picodmx
  psx_controller
  psx_emulation
  secondary_pico
  wii_extensions
  world_tour_drum
  psx_emulation
  wii_extension_emulation
  wii_remote_emulation
  SNESpad
  n64_gamecube_controller
  ws2812
  apa102
  stp16cpc
  nanopb
  hidparser
  hardware_i2c
  hardware_spi
  hardware_adc
  hardware_pio
  hardware_gpio
  hardware_pwm
  hardware_flash
  hardware_timer
  hardware_sleep
  pico_unique_id
  pico_bootsel_via_double_reset
  tinyusb_host
  tinyusb_device
  tinyusb_pico_pio_usb
  pico_fota_bootloader_lib
  tinyusb_device_unmarked)

pfb_compile_with_bootloader(${TARGET})

if (PICO_BOARD STREQUAL "pico2_w")
  set_property(TARGET ${TARGET} PROPERTY PICOTOOL_EXTRA_UF2_ARGS --platform rp2350)
endif()
pico_add_extra_outputs(${TARGET})

add_custom_target(build_combined ALL
    DEPENDS pico_fota_bootloader ${TARGET}
)
add_custom_command(
        TARGET build_combined
        POST_BUILD
        COMMAND python ${CMAKE_SOURCE_DIR}/build.py "${GIT_COMMIT}" "${CMAKE_CURRENT_BINARY_DIR}" "${PICO_BOARD}"
)

if(NOT SANTROLLER_VARIANT_BUILD)
  add_custom_target(build_board_variants
    COMMAND ${CMAKE_COMMAND}
      -S "${CMAKE_SOURCE_DIR}"
      -B "${CMAKE_BINARY_DIR}/variants"
      -G "${CMAKE_GENERATOR}"
      -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
      -DCMAKE_EXPORT_COMPILE_COMMANDS=${CMAKE_EXPORT_COMPILE_COMMANDS}
      -DENABLE_WIFI=${ENABLE_WIFI}
      -DENABLE_XBOX_WIRELESS=${ENABLE_XBOX_WIRELESS}
      -DSANTROLLER_VARIANT_DRIVER=ON
    COMMAND ${CMAKE_COMMAND} --build "${CMAKE_BINARY_DIR}/variants" --parallel
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_BINARY_DIR}/variants/santroller_pico_w.uf2" "${CMAKE_BINARY_DIR}/santroller_pico_w.uf2"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_BINARY_DIR}/variants/santroller_pico_w_fota.uf2" "${CMAKE_BINARY_DIR}/santroller_pico_w_fota.uf2"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_BINARY_DIR}/variants/santroller_pico2_w.uf2" "${CMAKE_BINARY_DIR}/santroller_pico2_w.uf2"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_BINARY_DIR}/variants/santroller_pico2_w_fota.uf2" "${CMAKE_BINARY_DIR}/santroller_pico2_w_fota.uf2"
  )
endif()