# Package firmware binaries into a static flash image and header
# This script runs during CMake configuration

if(DEFINED SANTROLLER_SHARED_GENERATED_DIR)
    set(GENERATED_DIR "${SANTROLLER_SHARED_GENERATED_DIR}")
else()
    set(GENERATED_DIR "${CMAKE_BINARY_DIR}/generated")
endif()

set(FIRMWARE_DIR "${CMAKE_BINARY_DIR}/firmware")

file(MAKE_DIRECTORY "${GENERATED_DIR}")

find_package(Python3 COMPONENTS Interpreter REQUIRED)

message(STATUS "Packaging Xbox wireless dongle firmware into static flash image...")
execute_process(
    COMMAND ${Python3_EXECUTABLE} "${CMAKE_SOURCE_DIR}/tools/package_dongle_firmware.py"
        --firmware-dir "${FIRMWARE_DIR}"
        --output-dir "${GENERATED_DIR}"
        --partition-size-kb 96
        --flash-offset 0xA000
    RESULT_VARIABLE PACKAGE_RESULT
)

if(PACKAGE_RESULT EQUAL 0)
    set(FIRMWARE_DATA_AVAILABLE TRUE)
else()
    message(WARNING "Failed to package Xbox wireless dongle firmware")
    set(FIRMWARE_DATA_AVAILABLE FALSE)
endif()
