#pragma once
// Helpers for the config tests: the device's flash and its RAM cache, and uploading a config the way
// the config tool does (a ConfigInfo report, then the data in 63 byte feature reports)
#include <gtest/gtest.h>
#include <cstring>
#include <vector>
#include <pb_encode.h>
#include "CRC32.h"
#include "config.pb.h"
#include "config/FlashPROM.h"
#include "config/config_storage.hpp"
#include "config_fakes.hpp"

namespace config_test
{
// The magic the config tool writes into ConfigInfo (SettingsContext.ts)
constexpr uint32_t CONFIG_MAGIC = 0xd2f1e365;
// The config tool's feature reports carry 63 bytes after the report id
constexpr uint32_t REPORT_SIZE = 63;
// The footer at the very end of the config area: six little endian uint32s
constexpr uint32_t FOOTER_SIZE = 24;
// The most config data that fits in front of the footer
constexpr uint32_t MAX_DATA_SIZE = EEPROM_SIZE_BYTES - FOOTER_SIZE;

void ensure_core1_running();

inline uint8_t *flash_area() { return reinterpret_cast<uint8_t *>(EEPROM_ADDRESS_START); }

inline uint32_t crc32(const std::vector<uint8_t> &data)
{
    return CRC32::calculate(data.data(), static_cast<uint16_t>(data.size()));
}

struct Footer
{
    uint32_t data_size;
    uint32_t data_crc;
    uint32_t main_size;
    uint32_t aux_size;
    uint32_t magic;
    uint32_t current_profile;
};

inline void put_u32(uint8_t *at, uint32_t value)
{
    for (int i = 0; i < 4; i++)
    {
        at[i] = uint8_t(value >> (8 * i));
    }
}

inline uint32_t get_u32(const uint8_t *at)
{
    return uint32_t(at[0]) | uint32_t(at[1]) << 8 | uint32_t(at[2]) << 16 | uint32_t(at[3]) << 24;
}

// Write an image (data packed against the footer) straight into an area, the way read_flash expects it
inline void write_raw_image(uint8_t *area, const std::vector<uint8_t> &data, const Footer &footer)
{
    uint8_t *end = area + EEPROM_SIZE_BYTES;
    if (!data.empty())
    {
        memcpy(end - FOOTER_SIZE - data.size(), data.data(), data.size());
    }
    put_u32(end - 24, footer.data_size);
    put_u32(end - 20, footer.data_crc);
    put_u32(end - 16, footer.main_size);
    put_u32(end - 12, footer.aux_size);
    put_u32(end - 8, footer.magic);
    put_u32(end - 4, footer.current_profile);
}

inline Footer read_raw_footer(const uint8_t *area)
{
    const uint8_t *end = area + EEPROM_SIZE_BYTES;
    return {get_u32(end - 24), get_u32(end - 20), get_u32(end - 16), get_u32(end - 12), get_u32(end - 8),
            get_u32(end - 4)};
}

inline Footer valid_footer(const std::vector<uint8_t> &main, const std::vector<uint8_t> &aux)
{
    std::vector<uint8_t> data(main);
    data.insert(data.end(), aux.begin(), aux.end());
    return {uint32_t(data.size()), crc32(data), uint32_t(main.size()), uint32_t(aux.size()), CONFIG_MAGIC, 0};
}

inline std::vector<uint8_t> concat(const std::vector<uint8_t> &a, const std::vector<uint8_t> &b)
{
    std::vector<uint8_t> out(a);
    out.insert(out.end(), b.begin(), b.end());
    return out;
}

// A ConfigInfo report as the config tool sends it: length delimited, zero padded to the report size
inline std::vector<uint8_t> info_report(int32_t data_size, int32_t data_crc, int32_t main_size, int32_t aux_size,
                                        int32_t magic = int32_t(CONFIG_MAGIC))
{
    proto_ConfigInfo info = proto_ConfigInfo_init_zero;
    info.dataSize = data_size;
    info.dataCrc = data_crc;
    info.mainSize = main_size;
    info.auxSize = aux_size;
    info.magic = magic;
    std::vector<uint8_t> report(REPORT_SIZE, 0);
    pb_ostream_t stream = pb_ostream_from_buffer(report.data(), report.size());
    EXPECT_TRUE(pb_encode_delimited(&stream, proto_ConfigInfo_fields, &info));
    return report;
}

inline std::vector<uint8_t> info_report_for(const std::vector<uint8_t> &main, const std::vector<uint8_t> &aux)
{
    const Footer footer = valid_footer(main, aux);
    return info_report(int32_t(footer.data_size), int32_t(footer.data_crc), int32_t(footer.main_size),
                       int32_t(footer.aux_size));
}

// The data reports for an upload: REPORT_SIZE bytes each, the last one zero padded
inline std::vector<std::vector<uint8_t>> data_reports(const std::vector<uint8_t> &data)
{
    std::vector<std::vector<uint8_t>> reports;
    for (size_t start = 0; start < data.size(); start += REPORT_SIZE)
    {
        std::vector<uint8_t> report(REPORT_SIZE, 0);
        const size_t len = std::min<size_t>(REPORT_SIZE, data.size() - start);
        memcpy(report.data(), data.data() + start, len);
        reports.push_back(report);
    }
    return reports;
}

// Upload a config the way the tool does. Returns the result of the last chunk, or Done for an empty
// config, which the tool sends as just the info report.
inline ConfigStorage::WriteResult upload(ConfigStorage &storage, const std::vector<uint8_t> &main,
                                         const std::vector<uint8_t> &aux)
{
    const auto info = info_report_for(main, aux);
    EXPECT_TRUE(storage.write_info(info.data(), uint16_t(info.size())));
    const auto data = concat(main, aux);
    if (data.empty())
    {
        return ConfigStorage::WriteResult::Done;
    }
    ConfigStorage::WriteResult result = ConfigStorage::WriteResult::InProgress;
    uint32_t start = 0;
    for (const auto &report : data_reports(data))
    {
        result = storage.write_chunk(report.data(), uint16_t(report.size()), start);
        start += uint32_t(report.size());
    }
    return result;
}

// Some bytes that look nothing like a footer or erased flash
inline std::vector<uint8_t> pattern(size_t size, uint8_t seed = 1)
{
    std::vector<uint8_t> out(size);
    for (size_t i = 0; i < size; i++)
    {
        out[i] = uint8_t(seed + i * 7 + (i >> 8));
    }
    return out;
}

// Starts each test like a freshly booted device: erased flash, read into the cache
class StorageTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        ensure_core1_running();
        memset(fake_flash::memory, 0xFF, sizeof(fake_flash::memory));
        fake_flash::erase_count = 0;
        fake_flash::program_count = 0;
        fake_flash::program_without_erase = false;
        fake_time::now_us = 1000000;
        EEPROM.should_commit_at = 0;
        EEPROM.start();
        fake_loader::reset();
    }

    // Let the deferred commit run, like the main loop's EEPROM.tick() does
    void run_pending_commit()
    {
        fake_time::advance_ms(EEPROM_WRITE_WAIT + 1);
        EEPROM.tick();
    }

    // Put an image in flash as if a previous session had saved it, and boot
    void boot_with_flash_image(const std::vector<uint8_t> &main, const std::vector<uint8_t> &aux)
    {
        write_raw_image(flash_area(), concat(main, aux), valid_footer(main, aux));
        EEPROM.start();
    }

    ConfigStorage storage;
};
}
