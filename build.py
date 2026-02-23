import sys
import os
import struct

hash = sys.argv[1]
out_dir = sys.argv[2]

# write commit to version file
with open(os.path.join(out_dir,"commit.hash"), "w") as hash_file:
    hash_file.write(hash)

# concat bootloader and firmware uf2s, so that we have a single uf2 for the initial upload
UF2_FAMILY_ID = 0xE48BFF56
UF2_MAGIC_FIRST = 0x0A324655
UF2_MAGIC_SECOND = 0x9E5D5157
UF2_MAGIC_FINAL = 0x0AB16F30


def convert_binary_to_uf2(binaries: list[tuple[int, bytearray]]) -> bytearray:
    """Convert a GP2040-CE binary payload to Microsoft's UF2 format.

    https://github.com/microsoft/uf2/tree/master#overview

    Args:
        binaries: list of start,binary pairs of binary data to write at the specified memory offset in flash
    Returns:
        the content in UF2 format
    """
    total_blocks = sum([(len(binary) // 256) + 1 if len(binary) % 256 else len(binary) // 256
                        for offset, binary in binaries])
    block_count = 0

    uf2 = bytearray()
    for start, binary in binaries:
        size = len(binary)
        index = 0
        while index < size:
            pad_count = 476 - len(binary[index:index+256])
            uf2 += struct.pack('<LLLLLLLL',
                               UF2_MAGIC_FIRST,                                 # first magic number
                               UF2_MAGIC_SECOND,                                # second magic number
                               0x00002000,                                      # familyID present
                               0x10000000 + start + index,                      # address to write to
                               256,                                             # bytes to write in this block
                               block_count,                                     # sequential block number
                               total_blocks,                                    # total number of blocks
                               UF2_FAMILY_ID)                                   # family ID
            uf2 += binary[index:index+256] + bytearray(b'\x00' * pad_count)     # content
            uf2 += struct.pack('<L', UF2_MAGIC_FINAL)                           # final magic number
            index += 256
            block_count += 1
    return uf2


def convert_uf2_to_binary(uf2s: list[bytearray]) -> bytearray:
    """Convert a Microsoft's UF2 payload to a raw binary.

    https://github.com/microsoft/uf2/tree/master#overview

    Args:
        uf2: bytearray content to convert from a UF2 payload
    Returns:
        the content in sequential binary format
    """
    binary = bytearray()
    old_uf2_addr = None
    for uf2 in uf2s:
        if len(uf2) % 512 != 0:
            raise ValueError(f"provided binary is length {len(uf2)}, which isn't fully divisible by 512!")
        block_count = 0
        block_num = 0
        for index in range(0, len(uf2), 512):
            chunk = uf2[index:index+512]
            _, _, _, uf2_addr, bytes_, block_num, block_count, _ = struct.unpack('<LLLLLLLL', chunk[0:32])
            content = chunk[32:508]
            if block_num != index // 512:
                raise ValueError(f"inconsistent block number in reading UF2, got {block_num}, expected {index // 512}!")
            if block_count != len(uf2) // 512:
                raise ValueError(f"inconsistent block count in reading UF2, got {block_count}, expected {len(uf2) // 512}!")

            if old_uf2_addr and (uf2_addr >= old_uf2_addr + bytes_):
                # the new binary content is not immediately after what we wrote, it's further ahead, so pad
                # the difference
                binary += bytearray(b'\x00' * (uf2_addr - (old_uf2_addr + bytes_)))
            elif old_uf2_addr and (uf2_addr < old_uf2_addr + bytes_):
                # this is seeking backwards which we don't see yet
                raise NotImplementedError("going backwards in binary files is not yet supported")

            binary += content[0:bytes_]
            old_uf2_addr = uf2_addr

        # when this is all done we should have counted the expected number of blocks
        if block_count != block_num + 1:
            raise ValueError(f"not all expected blocks ({block_count}) were found, only got {block_num + 1}!")
    return binary

bootloader_filename = os.path.join(out_dir,"lib","pico_fota_bootloader","pico_fota_bootloader.uf2")
firmware_filename = os.path.join(out_dir,"santroller.uf2")
out_filename = os.path.join(out_dir,"santroller_fota.uf2")

with open(bootloader_filename, 'rb') as bootloader:
    with open(firmware_filename, 'rb') as app:
        bootloader_binary = convert_uf2_to_binary([bytearray(bootloader.read()), bytearray(app.read())])

with open(os.path.join(out_dir,"santroller_fota.uf2"), "wb") as full_file:
    full_file.write(convert_binary_to_uf2([(0, bootloader_binary)]))

print("Build combined uf2!")