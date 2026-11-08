#pragma once
// A 16 sector area one sector into the fake flash, with the same layout rules as the generated one
#include <hardware/flash.h>
#define RADIO_FW_FLASH_OFFSET 0x1000u
#define RADIO_FW_ADDRESS (XIP_BASE + RADIO_FW_FLASH_OFFSET)
#define RADIO_FW_SIZE 0x10000u
#define RADIO_FW_TOC_OFFSET 0xF000u
#define RADIO_FW_TOC_MAGIC 0x57465253u
#define RADIO_FW_TOC_FORMAT 1u
#define RADIO_FW_TOC_MAX_ENTRIES 32u
#define RADIO_FW_LEGACY_CYW43_OFFSET 0x4000u
#define RADIO_FW_LEGACY_CYW43_WIFI_LEN 0x2345u
#define RADIO_FW_LEGACY_CYW43_CLM_LEN 0x100u
