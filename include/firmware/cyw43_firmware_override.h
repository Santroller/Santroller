// Override CYW43 firmware with compressed version
#ifndef CYW43_FIRMWARE_OVERRIDE_H
#define CYW43_FIRMWARE_OVERRIDE_H

#ifdef USE_COMPRESSED_CYW43_FIRMWARE

#include "cyw43_firmware_compressed.h"

extern const unsigned char *cyw43_get_decompressed_firmware(void);
extern void cyw43_ensure_firmware_decompressed(void);

#ifndef CYW43_RESOURCE_ATTRIBUTE
#define CYW43_RESOURCE_ATTRIBUTE __attribute__((constructor))
#endif

#define wb43439A0_7_95_49_00_combined (cyw43_ensure_firmware_decompressed(), cyw43_get_decompressed_firmware())

extern uint32_t cyw43_firmware_decompressed_len;
#define CYW43_WIFI_FW_LEN cyw43_firmware_decompressed_len

#define fw_data ((uintptr_t)cyw43_get_decompressed_firmware())

#define CYW43_CLM_LEN 0

extern const unsigned char *cyw43_get_decompressed_btfw(void);
extern void cyw43_ensure_btfw_decompressed(void);
extern uint32_t cyw43_btfw_decompressed_len;

#define cyw43_btfw_43439 (cyw43_ensure_btfw_decompressed(), cyw43_get_decompressed_btfw())
#define cyw43_btfw_43439_len cyw43_btfw_decompressed_len
#define btfw_data ((uintptr_t)cyw43_get_decompressed_btfw())
#define btfw_len cyw43_btfw_decompressed_len

#else
#include "wb43439A0_7_95_49_00_combined.h"
#endif

#endif // CYW43_FIRMWARE_OVERRIDE_H
