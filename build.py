import sys
import os
import struct

hash = sys.argv[1]
out_dir = sys.argv[2]
family = sys.argv[3]
shared_gen_dir = sys.argv[4] if len(sys.argv) > 4 else None
# write commit to version file
with open(os.path.join(out_dir,"commit.hash"), "w") as hash_file:
    hash_file.write(hash)

from dataclasses import dataclass
import itertools
import struct
import sys

UF2_FAMILY_ID_PICO = 0xE48BFF56
UF2_FAMILY_ID_PICO_HEADER = 0xE48BFF57
UF2_FAMILY_ID_PICO2 = 0xE48BFF59
block_format = "< 8I 476B I"
magic0 = 0x0A324655 # "UF2\n"
magic1 = 0x9E5D5157
magic2 = 0x0AB16F30

block_size = 256
sector_size = 4 * 1024

@dataclass
class Uf2Block:
    # Fields are ordered as they appear in the serialized binary
    # DO NOT CHANGE
    flags: int
    address: int
    size: int
    seq: int
    total_blocks: int
    family_id: int
    data: bytes = bytes()

def read_file(name):
    with open(name, "rb") as f:
        data = f.read()
    return data

def decode_uf2(data):
    while data:
        block, data = data[:512], data[512:]
        block = struct.unpack(block_format, block)

        header, body, footer = block[:8], block[8:-1], block[-1:]
        assert (header[:2], footer) == ((magic0, magic1), (magic2,))

        block = Uf2Block(*header[2:])
        block.data = bytes(body[:block.size])

        yield block

def write_uf2(blocks, name):
    with open(name, "wb") as f:
        for seq, b in enumerate(blocks):
            assert len(b.data) == b.size
            data = struct.pack(
                block_format,
                magic0,
                magic1,
                b.flags,
                b.address,
                b.size,
                seq,
                len(blocks),
                b.family_id,
                *b.data.ljust(476, b"\x00"),
                magic2,
            )
            f.write(data)



bootloader_filename = os.path.join(out_dir,"lib","pico_fota_bootloader","pico_fota_bootloader.uf2")
firmware_filename = os.path.join(out_dir,"santroller.uf2")
out_filename = os.path.join(out_dir,"santroller_fota.uf2")
inputs = (read_file(f) for f in (bootloader_filename, firmware_filename))
inputs = (decode_uf2(f) for f in inputs)
blocks = [b for b in itertools.chain.from_iterable(inputs)]
header_blocks = [b for b in blocks if b.family_id == UF2_FAMILY_ID_PICO_HEADER]
blocks = [b for b in blocks if b.family_id != UF2_FAMILY_ID_PICO_HEADER]
ref = blocks[0]
for b in blocks:
    assert b.flags == ref.flags
    assert b.size == block_size
    assert b.family_id == ref.family_id

static_firmware_candidates = [
    os.path.join(out_dir, "static_firmware.bin"),
    os.path.join(out_dir, "generated", "static_firmware.bin"),
    os.path.join(out_dir, "..", "generated", "static_firmware.bin"),
]
if shared_gen_dir:
    static_firmware_candidates.insert(0, os.path.join(shared_gen_dir, "static_firmware.bin"))

static_firmware_path = next((p for p in static_firmware_candidates if os.path.isfile(p)), None)
if static_firmware_path:
    print(f"Found static firmware at {static_firmware_path}, adding to UF2")
    static_firmware_data = read_file(static_firmware_path)
    static_firmware_blocks = []
    STATIC_FIRMWARE_START = 0x1000A000
    for offset in range(0, len(static_firmware_data), block_size):
        chunk = static_firmware_data[offset:offset + block_size]
        if len(chunk) < block_size:
            chunk = chunk.ljust(block_size, b"\xFF")
        static_firmware_blocks.append(Uf2Block(
            ref.flags,
            STATIC_FIRMWARE_START + offset,
            block_size,
            0,
            0,
            ref.family_id,
            chunk
        ))
    write_uf2(static_firmware_blocks, os.path.join(out_dir, "static_firmware.uf2"))
    app_only_blocks = list(decode_uf2(read_file(firmware_filename)))
    write_uf2(app_only_blocks, os.path.join(out_dir, "santroller_app_only.uf2"))
    full_install_blocks = app_only_blocks + static_firmware_blocks
    write_uf2(full_install_blocks, firmware_filename)
    blocks += static_firmware_blocks

block_map = set(b.address for b in blocks)
sector_map = set(a // sector_size * sector_size for a in block_map)
blocks_to_fill = set(itertools.chain.from_iterable(
    (range(a, a + sector_size, block_size) for a in sector_map),
)).difference(block_map)
def padding_block(address):
    return Uf2Block(
        ref.flags,
        address,
        block_size,
        0, # dummy
        0, # dummy
        ref.family_id,
        bytes(block_size),
    )
blocks += (padding_block(a) for a in blocks_to_fill)

blocks.sort(key=lambda b: b.address)
for a, b in itertools.pairwise(blocks):
    # Ensure no blocks overlap
    assert a.address + a.size <= b.address
# header and footer blocks seem unneeded and stop things working so...
# if header_blocks:
#     header_block = header_blocks[0]
#     blocks.insert(0, header_blocks[0])
#     blocks.append(header_blocks[0])
#     print(blocks)

write_uf2(blocks, os.path.join(out_dir,"santroller_fota.uf2"))

print("Built combined uf2!")