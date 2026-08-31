# Compress CYW43 WiFi firmware to save flash space
# This creates a compressed version of the CYW43 firmware blob

set(CYW43_FIRMWARE_DIR "${PICO_SDK_PATH}/lib/cyw43-driver/firmware")
set(CYW43_FIRMWARE_HEADER "${CYW43_FIRMWARE_DIR}/wb43439A0_7_95_49_00_combined.h")
set(GENERATED_DIR "${CMAKE_BINARY_DIR}/generated")
set(CYW43_COMPRESSED_C "${GENERATED_DIR}/cyw43_firmware_compressed.c")
set(CYW43_COMPRESSED_H "${GENERATED_DIR}/cyw43_firmware_compressed.h")

# Create generated directory
file(MAKE_DIRECTORY "${GENERATED_DIR}")

# Find gzip
find_program(GZIP_EXECUTABLE gzip)
find_program(PYTHON_EXECUTABLE python3)

if(NOT GZIP_EXECUTABLE OR NOT PYTHON_EXECUTABLE)
    message(WARNING "gzip or python3 not found - CYW43 firmware will not be compressed")
    set(CYW43_FIRMWARE_COMPRESSED FALSE PARENT_SCOPE)
    return()
endif()

message(STATUS "Compressing CYW43 WiFi firmware...")

# Python script to extract and compress firmware
file(WRITE "${GENERATED_DIR}/compress_cyw43.py" "
import re
import gzip
import sys

# Read WiFi firmware header
with open('${CYW43_FIRMWARE_HEADER}', 'r') as f:
    wifi_content = f.read()

# Read BT firmware header
with open('${CYW43_FIRMWARE_DIR}/cyw43_btfw_43439.h', 'r') as f:
    bt_content = f.read()

# Extract WiFi firmware array
match = re.search(r'(?:static\\s+)?const\\s+(?:uint8_t|unsigned\\s+char)\\s+wb43439A0_7_95_49_00_combined\\[\\].*?=\\s*\\{([^}]+)\\}', wifi_content, re.DOTALL)
if not match:
    print('ERROR: Could not find WiFi firmware array', file=sys.stderr)
    sys.exit(1)

hex_data = match.group(1)
values = [int(x.strip(), 0) for x in hex_data.replace('\\n', ' ').split(',') if x.strip()]
wifi_firmware = bytes(values)

print(f'Original CYW43 WiFi firmware size: {len(wifi_firmware)} bytes')

# Extract BT firmware array
match = re.search(r'(?:static\\s+)?const\\s+(?:uint8_t|unsigned\\s+char)\\s+cyw43_btfw_43439\\[\\].*?=\\s*\\{([^}]+)\\}', bt_content, re.DOTALL)
if not match:
    print('ERROR: Could not find BT firmware array', file=sys.stderr)
    sys.exit(1)

hex_data = match.group(1)
values = [int(x.strip(), 0) for x in hex_data.replace('\\n', ' ').split(',') if x.strip()]
bt_firmware = bytes(values)

print(f'Original CYW43 BT firmware size: {len(bt_firmware)} bytes')

firmware = wifi_firmware  # Process WiFi first

# Compress
compressed = gzip.compress(firmware, compresslevel=9)
print(f'Compressed size: {len(compressed)} bytes ({len(compressed)*100//len(firmware)}%)')
print(f'Saved: {len(firmware) - len(compressed)} bytes')

# Write compressed binary
with open('${GENERATED_DIR}/cyw43_firmware.bin.gz', 'wb') as f:
    f.write(compressed)

# Write C array
with open('${CYW43_COMPRESSED_C}', 'w') as f:
    f.write('// Auto-generated compressed CYW43 firmware\\n')
    f.write('#include \"cyw43_firmware_compressed.h\"\\n\\n')
    f.write(f'const uint32_t cyw43_firmware_original_len = {len(firmware)};\\n')
    f.write(f'const uint32_t cyw43_firmware_compressed_len = {len(compressed)};\\n\\n')
    f.write('const uint8_t cyw43_firmware_compressed[] = {\\n')
    
    for i in range(0, len(compressed), 12):
        chunk = compressed[i:i+12]
        f.write('    ')
        f.write(', '.join(f'0x{b:02x}' for b in chunk))
        if i + 12 < len(compressed):
            f.write(',')
        f.write('\\n')
    
    f.write('};\\n')

# Write header
with open('${CYW43_COMPRESSED_H}', 'w') as f:
    f.write('// Auto-generated compressed CYW43 firmware\\n')
    f.write('#ifndef CYW43_FIRMWARE_COMPRESSED_H\\n')
    f.write('#define CYW43_FIRMWARE_COMPRESSED_H\\n\\n')
    f.write('#include <stdint.h>\\n\\n')
    f.write(f'extern const uint32_t cyw43_firmware_original_len;\\n')
    f.write(f'extern const uint32_t cyw43_firmware_compressed_len;\\n')
    f.write(f'extern const uint8_t cyw43_firmware_compressed[{len(compressed)}];\\n\\n')

# Now compress BT firmware
compressed_bt = gzip.compress(bt_firmware, compresslevel=9)
print(f'Compressed BT size: {len(compressed_bt)} bytes ({len(compressed_bt)*100//len(bt_firmware)}%)')
print(f'Saved: {len(bt_firmware) - len(compressed_bt)} bytes')

# Add BT firmware to C file
with open('${CYW43_COMPRESSED_C}', 'a') as f:
    f.write(f'const uint32_t cyw43_btfw_original_len = {len(bt_firmware)};\\n')
    f.write(f'const uint32_t cyw43_btfw_compressed_len = {len(compressed_bt)};\\n\\n')
    f.write('const uint8_t cyw43_btfw_compressed[] = {\\n')
    
    for i in range(0, len(compressed_bt), 12):
        chunk = compressed_bt[i:i+12]
        f.write('    ')
        f.write(', '.join(f'0x{b:02x}' for b in chunk))
        if i + 12 < len(compressed_bt):
            f.write(',')
        f.write('\\n')
    
    f.write('};\\n')

# Add BT firmware to header
with open('${CYW43_COMPRESSED_H}', 'a') as f:
    f.write(f'extern const uint32_t cyw43_btfw_original_len;\\n')
    f.write(f'extern const uint32_t cyw43_btfw_compressed_len;\\n')
    f.write(f'extern const uint8_t cyw43_btfw_compressed[{len(compressed_bt)}];\\n\\n')
    f.write('#endif\\n')

print('Generated compressed WiFi and BT firmware files')
")

# Run compression script
execute_process(
    COMMAND ${PYTHON_EXECUTABLE} "${GENERATED_DIR}/compress_cyw43.py"
    RESULT_VARIABLE COMPRESS_RESULT
    OUTPUT_VARIABLE COMPRESS_OUTPUT
    ERROR_VARIABLE COMPRESS_ERROR
)

if(COMPRESS_RESULT EQUAL 0)
    message(STATUS "${COMPRESS_OUTPUT}")
    set(CYW43_FIRMWARE_COMPRESSED TRUE CACHE BOOL "CYW43 firmware is compressed" FORCE)
else()
    message(WARNING "CYW43 firmware compression failed: ${COMPRESS_ERROR}")
    set(CYW43_FIRMWARE_COMPRESSED FALSE CACHE BOOL "CYW43 firmware is compressed" FORCE)
endif()
