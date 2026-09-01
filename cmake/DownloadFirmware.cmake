# Download Xbox Wireless Adapter Firmware (All Variants)
# This script downloads all firmware variants from Microsoft's servers during CMake configuration
# Based on: https://github.com/dlundqvist/xone/blob/master/install/firmware.sh

set(FIRMWARE_DIR "${CMAKE_BINARY_DIR}/firmware")

# All four firmware variants from xone driver
# Define each variant separately to avoid parsing issues
set(FIRMWARE_02E6_PID "02e6")
set(FIRMWARE_02E6_URL "https://catalog.s.download.windowsupdate.com/d/msdownload/update/driver/drvs/2017/03/2ea9591b-f751-442c-80ce-8f4692cdc67b_6b555a3a288153cf04aec6e03cba360afe2fce34.cab")
set(FIRMWARE_02E6_FILE "FW_ACC_00U.bin")
set(FIRMWARE_02E6_SHA "080ce4091e53a4ef3e5fe29939f51fd91f46d6a88be6d67eb6e99a5723b3a223")

set(FIRMWARE_02FE_PID "02fe")
set(FIRMWARE_02FE_URL "https://catalog.s.download.windowsupdate.com/c/msdownload/update/driver/drvs/2017/07/1cd6a87c-623f-4407-a52d-c31be49e925c_e19f60808bdcbfbd3c3df6be3e71ffc52e43261e.cab")
set(FIRMWARE_02FE_FILE "FW_ACC_00U.bin")
set(FIRMWARE_02FE_SHA "48084d9fa53b9bb04358f3bb127b7495dc8f7bb0b3ca1437bd24ef2b6eabdf66")

# Only include external USB dongles (02e6, 02fe)
# Excluded: 02f9 (Xbox One S built-in), 091e (Xbox One X built-in)
set(FIRMWARE_PIDS "02e6" "02fe")

# Create firmware directory
file(MAKE_DIRECTORY "${FIRMWARE_DIR}")

# Function to download and verify a single firmware
function(download_firmware PID URL CAB_FILENAME EXPECTED_SHA256)
    set(OUTPUT_FILE "${FIRMWARE_DIR}/xone_dongle_${PID}.bin")
    
    # Check if firmware already exists and is valid
    if(EXISTS "${OUTPUT_FILE}")
        file(SHA256 "${OUTPUT_FILE}" EXISTING_SHA256)
        if("${EXISTING_SHA256}" STREQUAL "${EXPECTED_SHA256}")
            message(STATUS "  [${PID}] Already downloaded and verified")
            return()
        else()
            message(STATUS "  [${PID}] Checksum mismatch, re-downloading...")
            file(REMOVE "${OUTPUT_FILE}")
        endif()
    endif()
    
    message(STATUS "  [${PID}] Downloading firmware...")
    
    # Download the CAB file
    set(CAB_FILE "${FIRMWARE_DIR}/driver_${PID}.cab")
    file(DOWNLOAD "${URL}" "${CAB_FILE}"
         SHOW_PROGRESS
         STATUS DOWNLOAD_STATUS
         TIMEOUT 60)
    
    list(GET DOWNLOAD_STATUS 0 DOWNLOAD_ERROR)
    if(NOT DOWNLOAD_ERROR EQUAL 0)
        list(GET DOWNLOAD_STATUS 1 DOWNLOAD_ERROR_MSG)
        message(WARNING "  [${PID}] Download failed: ${DOWNLOAD_ERROR_MSG}")
        return()
    endif()
    
    # Extract firmware
    find_program(BSDTAR_EXECUTABLE bsdtar)
    find_program(CABEXTRACT_EXECUTABLE cabextract)
    find_program(7Z_EXECUTABLE 7z)
    
    set(EXTRACTION_SUCCESS FALSE)
    
    if(BSDTAR_EXECUTABLE)
        execute_process(
                COMMAND ${BSDTAR_EXECUTABLE} -xf "${CAB_FILE}" -C "${FIRMWARE_DIR}"
            RESULT_VARIABLE EXTRACT_RESULT
            OUTPUT_QUIET ERROR_QUIET
        )
        if(EXTRACT_RESULT EQUAL 0)
            set(EXTRACTION_SUCCESS TRUE)
        endif()
    elseif(CABEXTRACT_EXECUTABLE)
        execute_process(
            COMMAND ${CABEXTRACT_EXECUTABLE} -d "${FIRMWARE_DIR}" "${CAB_FILE}"
            RESULT_VARIABLE EXTRACT_RESULT
            OUTPUT_QUIET ERROR_QUIET
        )
        if(EXTRACT_RESULT EQUAL 0)
            set(EXTRACTION_SUCCESS TRUE)
        endif()
    elseif(7Z_EXECUTABLE)
        execute_process(
            COMMAND ${7Z_EXECUTABLE} e "${CAB_FILE}" -o"${FIRMWARE_DIR}" "${CAB_FILENAME}" -y
            RESULT_VARIABLE EXTRACT_RESULT
            OUTPUT_QUIET ERROR_QUIET
        )
        if(EXTRACT_RESULT EQUAL 0)
            set(EXTRACTION_SUCCESS TRUE)
        endif()
    endif()
    
    # Clean up CAB file
    file(REMOVE "${CAB_FILE}")
    
    if(NOT EXTRACTION_SUCCESS)
        message(WARNING "  [${PID}] Extraction failed (no tool available)")
        return()
    endif()
    
    # Rename extracted file to standard name
        set(EXTRACTED_FILE "${FIRMWARE_DIR}/${CAB_FILENAME}")
        if(NOT EXISTS "${EXTRACTED_FILE}")
            file(GLOB_RECURSE EXTRACTED_FILES "${FIRMWARE_DIR}/*")
            string(TOLOWER "${CAB_FILENAME}" CAB_FILENAME_LOWER)
            foreach(CANDIDATE ${EXTRACTED_FILES})
                get_filename_component(CANDIDATE_NAME "${CANDIDATE}" NAME)
                string(TOLOWER "${CANDIDATE_NAME}" CANDIDATE_NAME_LOWER)
                if(CANDIDATE_NAME_LOWER STREQUAL CAB_FILENAME_LOWER)
                    set(EXTRACTED_FILE "${CANDIDATE}")
                    break()
                endif()
            endforeach()
        endif()
    if(EXISTS "${EXTRACTED_FILE}")
        file(RENAME "${EXTRACTED_FILE}" "${OUTPUT_FILE}")
    else()
        message(WARNING "  [${PID}] Extracted file not found: ${CAB_FILENAME}")
        return()
    endif()
    
    # Verify checksum
    file(SHA256 "${OUTPUT_FILE}" DOWNLOADED_SHA256)
    if(NOT "${DOWNLOADED_SHA256}" STREQUAL "${EXPECTED_SHA256}")
        message(WARNING "  [${PID}] Checksum verification failed!")
        message(WARNING "    Expected: ${EXPECTED_SHA256}")
        message(WARNING "    Got:      ${DOWNLOADED_SHA256}")
        file(REMOVE "${OUTPUT_FILE}")
        return()
    endif()
    
    message(STATUS "  [${PID}] Downloaded and verified successfully!")
endfunction()

message(STATUS "Downloading Xbox Wireless Adapter firmwares...")

# Download all firmware variants
set(FIRMWARE_COUNT 0)
foreach(PID_SUFFIX ${FIRMWARE_PIDS})
    string(TOUPPER "${PID_SUFFIX}" PID_UPPER)
    string(REPLACE "e" "E" PID_UPPER "${PID_UPPER}")
    
    download_firmware(
        "${FIRMWARE_${PID_UPPER}_PID}"
        "${FIRMWARE_${PID_UPPER}_URL}"
        "${FIRMWARE_${PID_UPPER}_FILE}"
        "${FIRMWARE_${PID_UPPER}_SHA}"
    )
    
    # Check if download was successful
    if(EXISTS "${FIRMWARE_DIR}/xone_dongle_${PID_SUFFIX}.bin")
        math(EXPR FIRMWARE_COUNT "${FIRMWARE_COUNT} + 1")
    endif()
endforeach()

# Check if at least one firmware was downloaded
list(LENGTH FIRMWARE_PIDS TOTAL_FIRMWARE_COUNT)
if(FIRMWARE_COUNT GREATER 0)
    message(STATUS "Successfully downloaded ${FIRMWARE_COUNT}/${TOTAL_FIRMWARE_COUNT} firmware variants")
    set(FIRMWARE_AVAILABLE TRUE)
else()
    message(WARNING "No firmwares downloaded successfully")
    message(WARNING "You can manually download firmware using: scripts/download_firmware.sh")
    set(FIRMWARE_AVAILABLE FALSE)
endif()
