#!/usr/bin/env python3
"""Package Xbox Wireless and CYW43 firmware into the static flash partition."""

import argparse
import gzip
import os
import re
import struct
import sys

HEADER_FORMAT = "< 9I 7I"
MAGIC = 0x31425821  # "!XB1"
VERSION = 2
HEADER_SIZE = 64


def read_cyw43_firmware(header_path):
    with open(header_path, "r", encoding="utf-8") as firmware_header:
        content = firmware_header.read()

    array_match = re.search(
        r"wb43439A0_7_95_49_00_combined\[\].*?=\s*\{([^}]+)\}",
        content,
        re.DOTALL,
    )
    wifi_len_match = re.search(r"#define\s+CYW43_WIFI_FW_LEN\s+\((\d+)\)", content)
    clm_len_match = re.search(r"#define\s+CYW43_CLM_LEN\s+\((\d+)\)", content)
    if not array_match or not wifi_len_match or not clm_len_match:
        raise ValueError(f"Could not parse CYW43 firmware header: {header_path}")

    firmware = bytes(
        int(value, 16)
        for value in re.findall(r"0x([0-9a-fA-F]{1,2})", array_match.group(1))
    )
    wifi_len = int(wifi_len_match.group(1))
    clm_len = int(clm_len_match.group(1))
    expected_len = ((wifi_len + 511) // 512) * 512 + clm_len
    if len(firmware) != expected_len:
        raise ValueError(
            f"CYW43 firmware size mismatch: found {len(firmware)}, expected {expected_len}"
        )
    return firmware, wifi_len, clm_len


def main():
    parser = argparse.ArgumentParser(
        description="Package firmware blobs into the static flash image."
    )
    parser.add_argument("--firmware-dir", required=True, help="Directory with Xbox dongle firmware")
    parser.add_argument("--output-dir", required=True, help="Directory for generated files")
    parser.add_argument("--partition-size-kb", type=int, required=True, help="Static partition size")
    parser.add_argument(
        "--xbox-partition-size-kb",
        type=int,
        default=96,
        help="Reserved Xbox dongle firmware area size",
    )
    parser.add_argument(
        "--flash-offset",
        type=lambda value: int(value, 0),
        default=0xA000,
        help="Static partition flash offset",
    )
    parser.add_argument("--cyw43-header", help="CYW43 combined firmware header to extract")
    args = parser.parse_args()

    os.makedirs(args.output_dir, exist_ok=True)
    partition_size_bytes = args.partition_size_kb * 1024
    xbox_partition_size_bytes = args.xbox_partition_size_kb * 1024
    if xbox_partition_size_bytes > partition_size_bytes:
        raise ValueError("Xbox firmware area exceeds the static partition size")

    firmware_paths = [
        os.path.join(args.firmware_dir, f"xone_dongle_{pid}.bin")
        for pid in ("02e6", "02fe")
    ]
    have_firmware = all(os.path.isfile(path) for path in firmware_paths)
    if xbox_partition_size_bytes and not have_firmware:
        print(
            f"Warning: Xbox dongle firmware missing in {args.firmware_dir}",
            file=sys.stderr,
        )

    dongle_data = [b"", b""]
    compressed_data = [b"", b""]
    if xbox_partition_size_bytes and have_firmware:
        for index, path in enumerate(firmware_paths):
            with open(path, "rb") as firmware_file:
                dongle_data[index] = firmware_file.read()
            compressed_data[index] = gzip.compress(
                dongle_data[index], compresslevel=9, mtime=0
            )

    offset_02e6 = HEADER_SIZE
    offset_02fe = (offset_02e6 + len(compressed_data[0]) + 3) & ~3
    total_data_len = offset_02fe + len(compressed_data[1])
    if xbox_partition_size_bytes and total_data_len > xbox_partition_size_bytes:
        raise ValueError(
            f"Compressed Xbox firmware ({total_data_len} bytes) exceeds "
            f"its {xbox_partition_size_bytes}-byte area"
        )

    header_bytes = struct.pack(
        HEADER_FORMAT,
        MAGIC if xbox_partition_size_bytes and have_firmware else 0,
        VERSION if xbox_partition_size_bytes and have_firmware else 0,
        xbox_partition_size_bytes,
        offset_02e6,
        len(compressed_data[0]),
        len(dongle_data[0]),
        offset_02fe,
        len(compressed_data[1]),
        len(dongle_data[1]),
        *([0] * 7),
    )
    binary = bytearray(b"\xFF" * partition_size_bytes)
    if xbox_partition_size_bytes:
        xbox_binary = bytearray(header_bytes)
        xbox_binary.extend(compressed_data[0])
        xbox_binary.extend(b"\x00" * (offset_02fe - len(xbox_binary)))
        xbox_binary.extend(compressed_data[1])
        xbox_binary.extend(b"\xFF" * (xbox_partition_size_bytes - len(xbox_binary)))
        binary[:xbox_partition_size_bytes] = xbox_binary

    if args.cyw43_header:
        cyw43_data, wifi_len, clm_len = read_cyw43_firmware(args.cyw43_header)
        cyw43_offset = (xbox_partition_size_bytes + 3) & ~3
        if cyw43_offset + len(cyw43_data) > partition_size_bytes:
            raise ValueError("CYW43 firmware exceeds the static partition size")
        binary[cyw43_offset : cyw43_offset + len(cyw43_data)] = cyw43_data

        cyw43_address = 0x10000000 + args.flash_offset + cyw43_offset
        cyw43_header_path = os.path.join(args.output_dir, "cyw43_static_firmware.h")
        with open(cyw43_header_path, "w", encoding="utf-8") as firmware_header:
            firmware_header.write(
                "#pragma once\n"
                "#include <stdint.h>\n"
                f"#define CYW43_WIFI_FW_LEN ({wifi_len})\n"
                f"#define CYW43_CLM_LEN ({clm_len})\n"
                f"const uintptr_t fw_data = (uintptr_t)0x{cyw43_address:08X};\n"
            )
        print(
            f"Packaged CYW43 firmware ({len(cyw43_data)} bytes) "
            f"at 0x{cyw43_address:08X}"
        )

    binary_path = os.path.join(args.output_dir, "static_firmware.bin")
    with open(binary_path, "wb") as firmware_image:
        firmware_image.write(binary)
    print(f"Wrote {binary_path} ({len(binary)} bytes)")

    flash_address = 0x10000000 + args.flash_offset
    header_path = os.path.join(args.output_dir, "xbox_dongle_firmware.h")
    header_content = f"""/* Auto-generated by package_dongle_firmware.py - DO NOT EDIT */
#ifndef XBOX_DONGLE_FIRMWARE_H
#define XBOX_DONGLE_FIRMWARE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define XBOX_STATIC_FIRMWARE_MAGIC 0x{MAGIC:08X}
#define XBOX_STATIC_FIRMWARE_VERSION {VERSION}
#define XBOX_STATIC_FIRMWARE_OFFSET 0x{args.flash_offset:X}
#define XBOX_STATIC_FIRMWARE_ADDRESS 0x{flash_address:08X}
#define XBOX_STATIC_FIRMWARE_PARTITION_SIZE ({args.xbox_partition_size_kb} * 1024)

struct __attribute__((packed)) XboxStaticFirmwareHeader
{{
    uint32_t magic;
    uint32_t version;
    uint32_t total_size;
    uint32_t fw_02e6_offset;
    uint32_t fw_02e6_compressed_len;
    uint32_t fw_02e6_len;
    uint32_t fw_02fe_offset;
    uint32_t fw_02fe_compressed_len;
    uint32_t fw_02fe_len;
    uint32_t reserved[7];
}};

static inline const struct XboxStaticFirmwareHeader *xbox_get_static_firmware_header(void)
{{
    const struct XboxStaticFirmwareHeader *header =
        (const struct XboxStaticFirmwareHeader *)(XBOX_STATIC_FIRMWARE_ADDRESS);
    if (header->magic != XBOX_STATIC_FIRMWARE_MAGIC || header->version != XBOX_STATIC_FIRMWARE_VERSION)
    {{
        return NULL;
    }}
    return header;
}}

#endif // XBOX_DONGLE_FIRMWARE_H
"""
    with open(header_path, "w", encoding="utf-8") as firmware_header:
        firmware_header.write(header_content)
    print(f"Wrote {header_path}")


if __name__ == "__main__":
    main()
