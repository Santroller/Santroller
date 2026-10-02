# Package Xbox Wireless and CYW43 firmware into the static flash image
# This script runs during CMake configuration

if(DEFINED SANTROLLER_SHARED_GENERATED_DIR)
    set(GENERATED_DIR "${SANTROLLER_SHARED_GENERATED_DIR}")
else()
    set(GENERATED_DIR "${CMAKE_BINARY_DIR}/generated")
endif()

set(FIRMWARE_DIR "${CMAKE_BINARY_DIR}/firmware")

file(MAKE_DIRECTORY "${GENERATED_DIR}")

find_package(Python3 COMPONENTS Interpreter REQUIRED)

set(PACKAGE_ARGS
    --firmware-dir "${FIRMWARE_DIR}"
    --output-dir "${GENERATED_DIR}"
    --partition-size-kb "${PFB_STATIC_FIRMWARE_SIZE_KB}"
    --xbox-partition-size-kb "${XBOX_STATIC_FIRMWARE_SIZE_KB}"
    --flash-offset 0xA000
)
if(ENABLE_WIFI)
    list(APPEND PACKAGE_ARGS
        --cyw43-header "${PICO_SDK_PATH}/lib/cyw43-driver/firmware/wb43439A0_7_95_49_00_combined.h"
    )
endif()

message(STATUS "Packaging static firmware image...")
execute_process(
    COMMAND ${Python3_EXECUTABLE} "${CMAKE_SOURCE_DIR}/tools/package_dongle_firmware.py"
        ${PACKAGE_ARGS}
    RESULT_VARIABLE PACKAGE_RESULT
)

if(PACKAGE_RESULT EQUAL 0)
    set(FIRMWARE_DATA_AVAILABLE TRUE)
else()
    message(WARNING "Failed to package Xbox wireless dongle firmware")
    set(FIRMWARE_DATA_AVAILABLE FALSE)
endif()
