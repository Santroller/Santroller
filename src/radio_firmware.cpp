#include "radio_firmware.h"
#if __has_include("radio_firmware_layout.h")
#include "radio_firmware_layout.h"
#else
// Built without the Xbox adapter or the CYW43, so there's no area
#define RADIO_FW_ADDRESS 0u
#define RADIO_FW_FLASH_OFFSET 0u
#define RADIO_FW_SIZE 0u
#define RADIO_FW_TOC_OFFSET 0u
#define RADIO_FW_TOC_MAGIC 0x57465253u
#define RADIO_FW_TOC_FORMAT 1u
#define RADIO_FW_TOC_MAX_ENTRIES 32u
#define RADIO_FW_LEGACY_CYW43_OFFSET 0u
#define RADIO_FW_LEGACY_CYW43_WIFI_LEN 0u
#define RADIO_FW_LEGACY_CYW43_CLM_LEN 0u
#endif
#if defined(ENABLE_XBOX_WIRELESS) && ENABLE_XBOX_WIRELESS
#include "xbox_dongle_firmware.h"
#endif
#include "config/FlashPROM.h"
#include <hardware/flash.h>
#include <hardware/sync.h>
#include <stdio.h>
#include <string.h>
#include <uzlib.h>

namespace
{
struct __attribute__((packed)) TocHeader
{
    uint32_t magic;
    uint16_t format;
    uint16_t entry_count;
    uint32_t content_version;
    uint32_t area_size;
    uint32_t crc;
    uint32_t reserved[3];
};
struct __attribute__((packed)) TocEntry
{
    uint16_t type;
    uint8_t compression;
    uint8_t reserved0;
    uint32_t offset;
    uint32_t stored_len;
    uint32_t raw_len;
    uint32_t crc;
    uint32_t reserved1;
};
static_assert(sizeof(TocHeader) == 32);
static_assert(sizeof(TocEntry) == 24);
constexpr uint32_t toc_max_size = sizeof(TocHeader) + RADIO_FW_TOC_MAX_ENTRIES * sizeof(TocEntry);
static_assert(toc_max_size <= FLASH_SECTOR_SIZE - FLASH_PAGE_SIZE, "The table of contents runs into the write marker's page");

// The last word of the area while it's being written. The image always has 0xFF there, and a finished
// write programs it to 0, as it can't be erased without erasing the table of contents.
constexpr uint32_t writing_marker = 0x54495257; // "WRIT"
constexpr uint32_t marker_offset = RADIO_FW_SIZE - sizeof(uint32_t);
constexpr uint32_t marker_page_offset = RADIO_FW_SIZE - FLASH_PAGE_SIZE;

enum class AreaState : uint8_t
{
    Unknown,
    Toc,
    Legacy,
    Interrupted,
};

AreaState s_state = AreaState::Unknown;
bool s_writing = false;
alignas(4) uint8_t s_toc_page[FLASH_PAGE_SIZE];
bool s_have_toc_page = false;

const uint8_t *area()
{
    return reinterpret_cast<const uint8_t *>(RADIO_FW_ADDRESS);
}

uint32_t crc32(const void *data, uint32_t len, uint32_t crc = 0xFFFFFFFF)
{
    return uzlib_crc32(data, len, crc);
}

// Checks a table of contents. The first page comes from first_page, the rest from flash
bool toc_valid(const uint8_t *first_page)
{
    TocHeader header;
    memcpy(&header, first_page, sizeof(header));
    if (header.magic != RADIO_FW_TOC_MAGIC || header.format != RADIO_FW_TOC_FORMAT ||
        header.entry_count > RADIO_FW_TOC_MAX_ENTRIES || header.area_size != RADIO_FW_SIZE)
    {
        return false;
    }
    uint32_t size = sizeof(TocHeader) + header.entry_count * sizeof(TocEntry);
    uint8_t toc[toc_max_size];
    uint32_t from_page = size < FLASH_PAGE_SIZE ? size : FLASH_PAGE_SIZE;
    memcpy(toc, first_page, from_page);
    memcpy(toc + from_page, area() + RADIO_FW_TOC_OFFSET + from_page, size - from_page);
    reinterpret_cast<TocHeader *>(toc)->crc = 0;
    if ((crc32(toc, size) ^ 0xFFFFFFFF) != header.crc)
    {
        return false;
    }
    const TocEntry *entries = reinterpret_cast<const TocEntry *>(toc + sizeof(TocHeader));
    for (uint32_t i = 0; i < header.entry_count; i++)
    {
        TocEntry entry;
        memcpy(&entry, &entries[i], sizeof(entry));
        if (entry.offset > RADIO_FW_TOC_OFFSET || entry.stored_len > RADIO_FW_TOC_OFFSET - entry.offset)
        {
            return false;
        }
    }
    return true;
}

const TocHeader *toc_header()
{
    return reinterpret_cast<const TocHeader *>(area() + RADIO_FW_TOC_OFFSET);
}

AreaState state()
{
    if (s_state != AreaState::Unknown)
    {
        return s_state;
    }
    if (RADIO_FW_SIZE == 0)
    {
        s_state = AreaState::Interrupted;
    }
    else if (toc_valid(area() + RADIO_FW_TOC_OFFSET))
    {
        s_state = AreaState::Toc;
    }
    else
    {
        uint32_t marker;
        memcpy(&marker, area() + marker_offset, sizeof(marker));
        s_state = marker == writing_marker ? AreaState::Interrupted : AreaState::Legacy;
    }
    return s_state;
}

bool find_in_toc(RadioFirmwareType type, RadioFirmwareBlob *out)
{
    const TocHeader *header = toc_header();
    const TocEntry *entries = reinterpret_cast<const TocEntry *>(header + 1);
    for (uint32_t i = 0; i < header->entry_count; i++)
    {
        if (entries[i].type != type)
        {
            continue;
        }
        out->data = area() + entries[i].offset;
        out->stored_len = entries[i].stored_len;
        out->raw_len = entries[i].raw_len;
        out->compression = entries[i].compression;
        return true;
    }
    return false;
}

bool find_legacy(RadioFirmwareType type, RadioFirmwareBlob *out)
{
    switch (type)
    {
    case RADIO_FW_XBOX_02E6:
    case RADIO_FW_XBOX_02FE: {
#if defined(ENABLE_XBOX_WIRELESS) && ENABLE_XBOX_WIRELESS
        const XboxStaticFirmwareHeader *header = xbox_get_static_firmware_header();
        if (!header)
        {
            return false;
        }
        bool fe = type == RADIO_FW_XBOX_02FE;
        out->data = area() + (fe ? header->fw_02fe_offset : header->fw_02e6_offset);
        out->stored_len = fe ? header->fw_02fe_compressed_len : header->fw_02e6_compressed_len;
        out->raw_len = fe ? header->fw_02fe_len : header->fw_02e6_len;
        out->compression = RADIO_FW_COMPRESSION_GZIP;
        return true;
#else
        return false;
#endif
    }
    case RADIO_FW_CYW43_WIFI:
    case RADIO_FW_CYW43_CLM: {
        // Nothing marks it, so this is where older firmware would read it from
        if (!RADIO_FW_LEGACY_CYW43_WIFI_LEN)
        {
            return false;
        }
        uint32_t offset = RADIO_FW_LEGACY_CYW43_OFFSET;
        uint32_t len = RADIO_FW_LEGACY_CYW43_WIFI_LEN;
        if (type == RADIO_FW_CYW43_CLM)
        {
            offset += (RADIO_FW_LEGACY_CYW43_WIFI_LEN + 511) & ~511u;
            len = RADIO_FW_LEGACY_CYW43_CLM_LEN;
        }
        out->data = area() + offset;
        out->stored_len = len;
        out->raw_len = len;
        out->compression = RADIO_FW_COMPRESSION_NONE;
        return true;
    }
    }
    return false;
}

void erase_sector(uint32_t offset)
{
    uint32_t status = save_and_disable_interrupts();
    flash_range_erase(RADIO_FW_FLASH_OFFSET + offset, FLASH_SECTOR_SIZE);
    restore_interrupts(status);
}

void program_page(uint32_t offset, const uint8_t *data)
{
    uint32_t status = save_and_disable_interrupts();
    flash_range_program(RADIO_FW_FLASH_OFFSET + offset, data, FLASH_PAGE_SIZE);
    restore_interrupts(status);
}

bool all_ff(const uint8_t *data, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++)
    {
        if (data[i] != 0xFF)
        {
            return false;
        }
    }
    return true;
}
} // namespace

bool radio_fw_find(RadioFirmwareType type, RadioFirmwareBlob *out)
{
    if (s_writing)
    {
        return false;
    }
    switch (state())
    {
    case AreaState::Toc:
        return find_in_toc(type, out);
    case AreaState::Legacy:
        return find_legacy(type, out);
    default:
        return false;
    }
}

uint32_t radio_fw_content_version(void)
{
    if (s_writing || state() != AreaState::Toc)
    {
        return 0;
    }
    return toc_header()->content_version;
}

uint32_t radio_fw_area_size(void)
{
    return RADIO_FW_SIZE;
}

bool radio_fw_writing(void)
{
    return s_writing;
}

bool radio_fw_write_begin(uint32_t size)
{
    if (RADIO_FW_SIZE == 0 || size != RADIO_FW_SIZE)
    {
        printf("radio fw: image is %lu bytes, the area is %u\r\n", (unsigned long)size, RADIO_FW_SIZE);
        return false;
    }
    s_writing = true;
    s_have_toc_page = false;
    s_state = AreaState::Unknown;
    // From here on the area reads as interrupted until the table of contents is written
    uint8_t marker_page[FLASH_PAGE_SIZE];
    memset(marker_page, 0xFF, sizeof(marker_page));
    memcpy(marker_page + FLASH_PAGE_SIZE - sizeof(uint32_t), &writing_marker, sizeof(uint32_t));
    flash_write_begin();
    erase_sector(RADIO_FW_TOC_OFFSET);
    program_page(marker_page_offset, marker_page);
    flash_write_end();
    return true;
}

bool radio_fw_write_block(uint32_t offset, const uint8_t *data, uint32_t len)
{
    if (!s_writing || offset % FLASH_SECTOR_SIZE || len % FLASH_PAGE_SIZE || offset > RADIO_FW_SIZE ||
        len > RADIO_FW_SIZE - offset)
    {
        printf("radio fw: bad block 0x%lx+%lu\r\n", (unsigned long)offset, (unsigned long)len);
        return false;
    }
    flash_write_begin();
    for (uint32_t sector = 0; sector < len; sector += FLASH_SECTOR_SIZE)
    {
        uint32_t sector_offset = offset + sector;
        uint32_t sector_len = len - sector < FLASH_SECTOR_SIZE ? len - sector : FLASH_SECTOR_SIZE;
        const uint8_t *sector_data = data + sector;
        bool toc_sector = sector_offset == RADIO_FW_TOC_OFFSET;
        if (!toc_sector)
        {
            if (memcmp(area() + sector_offset, sector_data, sector_len) == 0 &&
                all_ff(area() + sector_offset + sector_len, FLASH_SECTOR_SIZE - sector_len))
            {
                continue;
            }
            erase_sector(sector_offset);
        }
        for (uint32_t page = 0; page < sector_len; page += FLASH_PAGE_SIZE)
        {
            const uint8_t *page_data = sector_data + page;
            if (toc_sector && page == 0)
            {
                // Written by radio_fw_write_finish, once everything else checks out
                memcpy(s_toc_page, page_data, FLASH_PAGE_SIZE);
                s_have_toc_page = true;
                continue;
            }
            if (toc_sector && sector_offset + page == marker_page_offset)
            {
                // Erased at the start and holding the marker. The image has nothing here.
                if (!all_ff(page_data, FLASH_PAGE_SIZE))
                {
                    flash_write_end();
                    printf("radio fw: image has data in the marker page\r\n");
                    return false;
                }
                continue;
            }
            if (all_ff(page_data, FLASH_PAGE_SIZE))
            {
                continue;
            }
            program_page(sector_offset + page, page_data);
        }
    }
    flash_write_end();
    return true;
}

bool radio_fw_write_finish(void)
{
    if (!s_writing)
    {
        return false;
    }
    if (!s_have_toc_page || !toc_valid(s_toc_page))
    {
        printf("radio fw: image has no valid table of contents\r\n");
        radio_fw_write_abort();
        return false;
    }
    TocHeader header;
    memcpy(&header, s_toc_page, sizeof(header));
    for (uint32_t i = 0; i < header.entry_count; i++)
    {
        TocEntry entry;
        uint32_t entry_offset = sizeof(TocHeader) + i * sizeof(TocEntry);
        if (entry_offset + sizeof(entry) <= FLASH_PAGE_SIZE)
        {
            memcpy(&entry, s_toc_page + entry_offset, sizeof(entry));
        }
        else
        {
            // Straddling the first page or past it. Entries past it are already in flash
            uint8_t raw[sizeof(TocEntry)];
            for (uint32_t b = 0; b < sizeof(raw); b++)
            {
                uint32_t at = entry_offset + b;
                raw[b] = at < FLASH_PAGE_SIZE ? s_toc_page[at] : area()[RADIO_FW_TOC_OFFSET + at];
            }
            memcpy(&entry, raw, sizeof(entry));
        }
        uint32_t crc = crc32(area() + entry.offset, entry.stored_len) ^ 0xFFFFFFFF;
        if (crc != entry.crc)
        {
            printf("radio fw: entry %u (type %u) failed its crc\r\n", (unsigned)i, entry.type);
            radio_fw_write_abort();
            return false;
        }
    }
    uint8_t marker_page[FLASH_PAGE_SIZE];
    memset(marker_page, 0xFF, sizeof(marker_page));
    memset(marker_page + FLASH_PAGE_SIZE - sizeof(uint32_t), 0, sizeof(uint32_t));
    flash_write_begin();
    program_page(RADIO_FW_TOC_OFFSET, s_toc_page);
    program_page(marker_page_offset, marker_page);
    flash_write_end();
    s_writing = false;
    s_state = AreaState::Unknown;
    printf("radio fw: written, content version %lu\r\n", (unsigned long)radio_fw_content_version());
    return state() == AreaState::Toc;
}

void radio_fw_write_abort(void)
{
    s_writing = false;
    s_have_toc_page = false;
    s_state = AreaState::Unknown;
}

bool radio_fw_cyw43_present(void)
{
    RadioFirmwareBlob wifi;
    RadioFirmwareBlob clm;
    return radio_fw_find(RADIO_FW_CYW43_WIFI, &wifi) && radio_fw_find(RADIO_FW_CYW43_CLM, &clm) &&
           wifi.raw_len && radio_fw_cyw43_clm_len();
}

uintptr_t radio_fw_cyw43_address(void)
{
    RadioFirmwareBlob blob;
    if (radio_fw_find(RADIO_FW_CYW43_WIFI, &blob))
    {
        return reinterpret_cast<uintptr_t>(blob.data);
    }
    // cyw43-driver checks the firmware's trailer, so this fails cleanly there
    return RADIO_FW_ADDRESS + RADIO_FW_LEGACY_CYW43_OFFSET;
}

uint32_t radio_fw_cyw43_wifi_len(void)
{
    RadioFirmwareBlob blob;
    if (radio_fw_find(RADIO_FW_CYW43_WIFI, &blob))
    {
        return blob.raw_len;
    }
    return RADIO_FW_LEGACY_CYW43_WIFI_LEN;
}

uint32_t radio_fw_cyw43_clm_len(void)
{
    RadioFirmwareBlob blob;
    if (radio_fw_find(RADIO_FW_CYW43_CLM, &blob))
    {
        // The driver finds the CLM blob 512-byte aligned after the wifi firmware, not where the entry says
        uintptr_t expected = radio_fw_cyw43_address() + ((radio_fw_cyw43_wifi_len() + 511) & ~511u);
        if (reinterpret_cast<uintptr_t>(blob.data) != expected)
        {
            printf("radio fw: CYW43 CLM isn't where the driver will look for it\r\n");
            return 0;
        }
        return blob.raw_len;
    }
    return RADIO_FW_LEGACY_CYW43_CLM_LEN;
}
