// The radio firmware area: finding firmware through the table of contents or the older layout, and the
// configurator's writes, including ones that are interrupted or fail their checks
#include <gtest/gtest.h>
#include <cstring>
#include <vector>
#include <uzlib.h>
#include "radio_firmware.h"
#include "radio_firmware_layout.h"

namespace
{
uint8_t *area()
{
    return fake_flash::memory + RADIO_FW_FLASH_OFFSET;
}

uint32_t crc32(const uint8_t *data, size_t len)
{
    return uzlib_crc32(data, len, 0xFFFFFFFF) ^ 0xFFFFFFFF;
}

void put16(std::vector<uint8_t> &out, size_t at, uint16_t v)
{
    memcpy(out.data() + at, &v, 2);
}
void put32(std::vector<uint8_t> &out, size_t at, uint32_t v)
{
    memcpy(out.data() + at, &v, 4);
}

struct Entry
{
    uint16_t type;
    uint8_t compression;
    uint32_t offset;
    std::vector<uint8_t> data;
    uint32_t raw_len;
};

// An image laid out like package_dongle_firmware.py's
std::vector<uint8_t> make_image(const std::vector<Entry> &entries, uint32_t version)
{
    std::vector<uint8_t> image(RADIO_FW_SIZE, 0xFF);
    size_t toc = RADIO_FW_TOC_OFFSET;
    put32(image, toc + 0, RADIO_FW_TOC_MAGIC);
    put16(image, toc + 4, RADIO_FW_TOC_FORMAT);
    put16(image, toc + 6, entries.size());
    put32(image, toc + 8, version);
    put32(image, toc + 12, RADIO_FW_SIZE);
    put32(image, toc + 16, 0);
    memset(image.data() + toc + 20, 0, 12);
    for (size_t i = 0; i < entries.size(); i++)
    {
        const Entry &e = entries[i];
        memcpy(image.data() + e.offset, e.data.data(), e.data.size());
        size_t at = toc + 32 + i * 24;
        put16(image, at, e.type);
        image[at + 2] = e.compression;
        image[at + 3] = 0;
        put32(image, at + 4, e.offset);
        put32(image, at + 8, e.data.size());
        put32(image, at + 12, e.raw_len);
        put32(image, at + 16, crc32(e.data.data(), e.data.size()));
        put32(image, at + 20, 0);
    }
    put32(image, toc + 16, crc32(image.data() + toc, 32 + entries.size() * 24));
    return image;
}

std::vector<uint8_t> blob(size_t len, uint8_t seed)
{
    std::vector<uint8_t> out(len);
    for (size_t i = 0; i < len; i++)
    {
        out[i] = static_cast<uint8_t>(seed + i * 7);
    }
    return out;
}

std::vector<Entry> default_entries()
{
    uint32_t wifi_len = 0x2345;
    uint32_t clm_offset = 0x4000 + ((wifi_len + 511) & ~511u);
    return {
        {RADIO_FW_XBOX_02E6, RADIO_FW_COMPRESSION_GZIP, 0x40, blob(0x1800, 1), 0x3000},
        {RADIO_FW_XBOX_02FE, RADIO_FW_COMPRESSION_GZIP, 0x1840, blob(0x1900, 2), 0x3100},
        {RADIO_FW_CYW43_WIFI, RADIO_FW_COMPRESSION_NONE, 0x4000, blob(wifi_len, 3), wifi_len},
        {RADIO_FW_CYW43_CLM, RADIO_FW_COMPRESSION_NONE, clm_offset, blob(0x100, 4), 0x100},
    };
}

// The configurator sends the image in 4096 byte blocks
bool write_image(const std::vector<uint8_t> &image)
{
    if (!radio_fw_write_begin(image.size()))
    {
        return false;
    }
    for (uint32_t offset = 0; offset < image.size(); offset += FLASH_SECTOR_SIZE)
    {
        if (!radio_fw_write_block(offset, image.data() + offset, FLASH_SECTOR_SIZE))
        {
            radio_fw_write_abort();
            return false;
        }
    }
    return radio_fw_write_finish();
}

class RadioFirmware : public ::testing::Test
{
protected:
    void SetUp() override
    {
        memset(fake_flash::memory, 0xFF, sizeof(fake_flash::memory));
        fake_flash::program_without_erase = false;
        // Forgets what the area held
        radio_fw_write_abort();
    }

    void reload()
    {
        radio_fw_write_abort();
    }

    void expect_outside_untouched()
    {
        for (uint32_t i = 0; i < RADIO_FW_FLASH_OFFSET; i++)
        {
            ASSERT_EQ(fake_flash::memory[i], 0xFF) << "before the area at " << i;
        }
        for (uint32_t i = RADIO_FW_FLASH_OFFSET + RADIO_FW_SIZE; i < PICO_FLASH_SIZE_BYTES; i++)
        {
            ASSERT_EQ(fake_flash::memory[i], 0xFF) << "after the area at " << i;
        }
    }
};
} // namespace

TEST(RadioFirmwareCrc, MatchesZlib)
{
    const char *check = "123456789";
    EXPECT_EQ(crc32(reinterpret_cast<const uint8_t *>(check), 9), 0xCBF43926u);
}

TEST_F(RadioFirmware, EmptyAreaHasNoXboxFirmware)
{
    RadioFirmwareBlob found;
    EXPECT_FALSE(radio_fw_find(RADIO_FW_XBOX_02E6, &found));
    EXPECT_EQ(radio_fw_content_version(), 0u);
    EXPECT_EQ(radio_fw_area_size(), RADIO_FW_SIZE);
}

TEST_F(RadioFirmware, OlderLayoutStillFound)
{
    // Xbox header at 0 and CYW43 where the build put it, with no table of contents
    std::vector<uint8_t> header(64, 0);
    put32(header, 0, 0x31425821);
    put32(header, 4, 2);
    put32(header, 12, 0x40);
    put32(header, 16, 0x100);
    put32(header, 20, 0x200);
    put32(header, 24, 0x140);
    put32(header, 28, 0x110);
    put32(header, 32, 0x210);
    memcpy(area(), header.data(), header.size());
    reload();

    RadioFirmwareBlob found;
    ASSERT_TRUE(radio_fw_find(RADIO_FW_XBOX_02FE, &found));
    EXPECT_EQ(found.data, area() + 0x140);
    EXPECT_EQ(found.stored_len, 0x110u);
    EXPECT_EQ(found.raw_len, 0x210u);
    EXPECT_EQ(found.compression, RADIO_FW_COMPRESSION_GZIP);
    EXPECT_EQ(radio_fw_content_version(), 0u);
    EXPECT_EQ(radio_fw_cyw43_address(), RADIO_FW_ADDRESS + RADIO_FW_LEGACY_CYW43_OFFSET);
    EXPECT_EQ(radio_fw_cyw43_wifi_len(), RADIO_FW_LEGACY_CYW43_WIFI_LEN);
    EXPECT_EQ(radio_fw_cyw43_clm_len(), RADIO_FW_LEGACY_CYW43_CLM_LEN);
}

TEST_F(RadioFirmware, WritesAnImageAndFindsItsEntries)
{
    auto entries = default_entries();
    ASSERT_TRUE(write_image(make_image(entries, 7)));
    EXPECT_FALSE(fake_flash::program_without_erase);
    expect_outside_untouched();

    EXPECT_EQ(radio_fw_content_version(), 7u);
    for (const Entry &e : entries)
    {
        RadioFirmwareBlob found;
        ASSERT_TRUE(radio_fw_find(static_cast<RadioFirmwareType>(e.type), &found)) << e.type;
        EXPECT_EQ(found.data, area() + e.offset);
        EXPECT_EQ(found.stored_len, e.data.size());
        EXPECT_EQ(found.raw_len, e.raw_len);
        EXPECT_EQ(found.compression, e.compression);
        EXPECT_EQ(memcmp(found.data, e.data.data(), e.data.size()), 0);
    }
    EXPECT_EQ(radio_fw_cyw43_address(), RADIO_FW_ADDRESS + 0x4000);
    EXPECT_EQ(radio_fw_cyw43_wifi_len(), 0x2345u);
    EXPECT_EQ(radio_fw_cyw43_clm_len(), 0x100u);

    // and the same after a reboot
    reload();
    EXPECT_EQ(radio_fw_content_version(), 7u);
}

TEST_F(RadioFirmware, RewritesOverAnEarlierImage)
{
    ASSERT_TRUE(write_image(make_image(default_entries(), 1)));
    auto entries = default_entries();
    entries[0].data = blob(0x1700, 9);
    ASSERT_TRUE(write_image(make_image(entries, 2)));
    EXPECT_FALSE(fake_flash::program_without_erase);
    EXPECT_EQ(radio_fw_content_version(), 2u);
    RadioFirmwareBlob found;
    ASSERT_TRUE(radio_fw_find(RADIO_FW_XBOX_02E6, &found));
    EXPECT_EQ(found.stored_len, 0x1700u);
}

TEST_F(RadioFirmware, NothingFoundWhileWriting)
{
    ASSERT_TRUE(write_image(make_image(default_entries(), 1)));
    auto image = make_image(default_entries(), 2);
    ASSERT_TRUE(radio_fw_write_begin(image.size()));
    EXPECT_TRUE(radio_fw_writing());
    RadioFirmwareBlob found;
    EXPECT_FALSE(radio_fw_find(RADIO_FW_XBOX_02E6, &found));
    EXPECT_EQ(radio_fw_content_version(), 0u);
}

TEST_F(RadioFirmware, InterruptedWriteReadsAsMissing)
{
    ASSERT_TRUE(write_image(make_image(default_entries(), 1)));
    auto image = make_image(default_entries(), 2);
    ASSERT_TRUE(radio_fw_write_begin(image.size()));
    // the Xbox header sector made it, the rest didn't
    ASSERT_TRUE(radio_fw_write_block(0, image.data(), FLASH_SECTOR_SIZE));
    radio_fw_write_abort();

    // Not the older layout, even though the Xbox header is there
    for (int pass = 0; pass < 2; pass++)
    {
        RadioFirmwareBlob found;
        EXPECT_FALSE(radio_fw_find(RADIO_FW_XBOX_02E6, &found));
        EXPECT_FALSE(radio_fw_find(RADIO_FW_CYW43_WIFI, &found));
        EXPECT_EQ(radio_fw_content_version(), 0u);
        reload(); // and after a reboot
    }

    // and writing it again recovers
    ASSERT_TRUE(write_image(image));
    EXPECT_EQ(radio_fw_content_version(), 2u);
}

TEST_F(RadioFirmware, RefusesAnImageThatFailsItsCrc)
{
    auto image = make_image(default_entries(), 3);
    image[0x4000 + 5] ^= 0x10; // inside the CYW43 entry
    EXPECT_FALSE(write_image(image));
    EXPECT_FALSE(radio_fw_writing());
    RadioFirmwareBlob found;
    EXPECT_FALSE(radio_fw_find(RADIO_FW_XBOX_02E6, &found));
    EXPECT_EQ(radio_fw_content_version(), 0u);
}

TEST_F(RadioFirmware, RefusesAnImageWithoutATableOfContents)
{
    std::vector<uint8_t> image(RADIO_FW_SIZE, 0xFF);
    memcpy(image.data(), blob(0x100, 1).data(), 0x100);
    EXPECT_FALSE(write_image(image));
    EXPECT_EQ(radio_fw_content_version(), 0u);
}

TEST_F(RadioFirmware, RefusesAnImageWithACorruptTableOfContents)
{
    auto image = make_image(default_entries(), 3);
    image[RADIO_FW_TOC_OFFSET + 32 + 4] ^= 1; // first entry's offset, after the crc was taken
    EXPECT_FALSE(write_image(image));
}

TEST_F(RadioFirmware, RefusesTheWrongSize)
{
    EXPECT_FALSE(radio_fw_write_begin(RADIO_FW_SIZE - FLASH_SECTOR_SIZE));
    EXPECT_FALSE(radio_fw_write_begin(RADIO_FW_SIZE + FLASH_SECTOR_SIZE));
    EXPECT_FALSE(radio_fw_writing());
}

TEST_F(RadioFirmware, RefusesBlocksOutsideTheArea)
{
    std::vector<uint8_t> data(FLASH_SECTOR_SIZE, 0);
    ASSERT_TRUE(radio_fw_write_begin(RADIO_FW_SIZE));
    EXPECT_FALSE(radio_fw_write_block(RADIO_FW_SIZE, data.data(), FLASH_SECTOR_SIZE));
    EXPECT_FALSE(radio_fw_write_block(FLASH_PAGE_SIZE, data.data(), FLASH_PAGE_SIZE));
    EXPECT_FALSE(radio_fw_write_block(0, data.data(), 100));
    radio_fw_write_abort();
    expect_outside_untouched();
}

TEST_F(RadioFirmware, RefusesDataInTheMarkerPage)
{
    auto image = make_image(default_entries(), 3);
    image[RADIO_FW_SIZE - 1] = 0;
    EXPECT_FALSE(write_image(image));
}

TEST_F(RadioFirmware, ResentBlocksStillWrite)
{
    // A streamed upload sends a block again when part of it went missing
    auto image = make_image(default_entries(), 4);
    ASSERT_TRUE(radio_fw_write_begin(image.size()));
    for (uint32_t offset = 0; offset < image.size(); offset += FLASH_SECTOR_SIZE)
    {
        ASSERT_TRUE(radio_fw_write_block(offset, image.data() + offset, FLASH_SECTOR_SIZE));
        ASSERT_TRUE(radio_fw_write_block(offset, image.data() + offset, FLASH_SECTOR_SIZE));
    }
    ASSERT_TRUE(radio_fw_write_finish());
    EXPECT_EQ(radio_fw_content_version(), 4u);
}

TEST_F(RadioFirmware, ClmMustBeWhereTheDriverLooks)
{
    auto entries = default_entries();
    entries[3].offset += FLASH_PAGE_SIZE;
    ASSERT_TRUE(write_image(make_image(entries, 5)));
    EXPECT_EQ(radio_fw_cyw43_clm_len(), 0u);
}

TEST_F(RadioFirmware, ImageWithoutXboxFirmware)
{
    // eg a plain Pico's image, with something else where the CYW43 firmware usually is
    auto entries = default_entries();
    entries.erase(entries.begin(), entries.begin() + 2);
    ASSERT_TRUE(write_image(make_image(entries, 6)));
    RadioFirmwareBlob found;
    EXPECT_FALSE(radio_fw_find(RADIO_FW_XBOX_02E6, &found));
    EXPECT_TRUE(radio_fw_find(RADIO_FW_CYW43_WIFI, &found));
}

TEST_F(RadioFirmware, Cyw43PresentOnlyWithItsFirmware)
{
    // The older layout: wherever the build put it
    EXPECT_TRUE(radio_fw_cyw43_present());

    ASSERT_TRUE(write_image(make_image(default_entries(), 1)));
    EXPECT_TRUE(radio_fw_cyw43_present());

    auto no_cyw43 = default_entries();
    no_cyw43.resize(2);
    ASSERT_TRUE(write_image(make_image(no_cyw43, 2)));
    EXPECT_FALSE(radio_fw_cyw43_present());

    auto misplaced = default_entries();
    misplaced[3].offset += FLASH_PAGE_SIZE;
    ASSERT_TRUE(write_image(make_image(misplaced, 3)));
    EXPECT_FALSE(radio_fw_cyw43_present());

    // and after a write that didn't finish
    auto image = make_image(default_entries(), 4);
    ASSERT_TRUE(radio_fw_write_begin(image.size()));
    radio_fw_write_abort();
    EXPECT_FALSE(radio_fw_cyw43_present());
}
