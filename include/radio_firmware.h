#pragma once
// Firmware for radios: the Xbox wireless adapter's MT76 and the Pico W's CYW43, later USB bluetooth adapters.
// It lives in the static partition between the bootloader and the app, which OTA app updates don't touch.
//
// The partition's last sector holds a table of contents (written by tools/package_dongle_firmware.py):
//   header (32 bytes): u32 magic "SRFW", u16 format, u16 entry count, u32 content version, u32 area size,
//                      u32 crc32 of the header (this field zeroed) and the entries, u32 reserved[3]
//   entries (24 bytes): u16 type, u8 compression, u8 reserved, u32 offset, u32 stored length,
//                       u32 raw length, u32 crc32 of the stored bytes, u32 reserved
// Without one, the area has the older layout: the Xbox header at 0 and CYW43 straight after the Xbox area.
// The default image keeps that layout too, so firmware from before the table of contents still works.
//
// The configurator can rewrite the whole area over the config interface (FirmwareUpdate with target
// RADIO). A write marks the area as being written first and writes the table of contents last, so an
// interrupted write reads as missing rather than as the older layout.
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

enum RadioFirmwareType
{
    RADIO_FW_XBOX_02E6 = 1,
    RADIO_FW_XBOX_02FE = 2,
    RADIO_FW_CYW43_WIFI = 3,
    RADIO_FW_CYW43_CLM = 4,
};

enum RadioFirmwareCompression
{
    RADIO_FW_COMPRESSION_NONE = 0,
    RADIO_FW_COMPRESSION_GZIP = 1,
};

typedef struct
{
    const uint8_t *data;
    uint32_t stored_len;
    uint32_t raw_len;
    uint8_t compression;
} RadioFirmwareBlob;

// False while the area is being written
bool radio_fw_find(enum RadioFirmwareType type, RadioFirmwareBlob *out);
// 0 without a table of contents (the older layout, or nothing)
uint32_t radio_fw_content_version(void);
// Size of the area, 0 when this build has none (and can't write one)
uint32_t radio_fw_area_size(void);

// Writing. Blocks start on a 4096 byte boundary and are a multiple of 256 bytes long, in any order.
bool radio_fw_write_begin(uint32_t size);
bool radio_fw_write_block(uint32_t offset, const uint8_t *data, uint32_t len);
// Checks the table of contents and every entry, then writes the table of contents
bool radio_fw_write_finish(void);
// Leaves the area marked as interrupted
void radio_fw_write_abort(void);
bool radio_fw_writing(void);

// Whether there's CYW43 firmware to start it with. Without it (nothing in the table of contents, or a write
// that didn't finish) the driver would be handed whatever is where the older layout kept it.
bool radio_fw_cyw43_present(void);
// For cyw43-driver, through cyw43_static_firmware.h
uintptr_t radio_fw_cyw43_address(void);
uint32_t radio_fw_cyw43_wifi_len(void);
uint32_t radio_fw_cyw43_clm_len(void);

#ifdef __cplusplus
}
#endif
