#pragma once
// The pre-table of contents Xbox header, as package_dongle_firmware.py generates it
#include <stddef.h>
#include <stdint.h>
#include "radio_firmware_layout.h"

#define XBOX_STATIC_FIRMWARE_MAGIC 0x31425821
#define XBOX_STATIC_FIRMWARE_VERSION 2
#define XBOX_STATIC_FIRMWARE_ADDRESS RADIO_FW_ADDRESS

struct __attribute__((packed)) XboxStaticFirmwareHeader
{
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
};

static inline const struct XboxStaticFirmwareHeader *xbox_get_static_firmware_header(void)
{
    const struct XboxStaticFirmwareHeader *header =
        (const struct XboxStaticFirmwareHeader *)(XBOX_STATIC_FIRMWARE_ADDRESS);
    if (header->magic != XBOX_STATIC_FIRMWARE_MAGIC || header->version != XBOX_STATIC_FIRMWARE_VERSION)
    {
        return NULL;
    }
    return header;
}
