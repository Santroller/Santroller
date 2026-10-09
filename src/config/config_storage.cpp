#include "config/config_storage.hpp"

#include "CRC32.h"
#include "config/FlashPROM.h"
#include "config.pb.h"
#include "tusb.h"
#include <algorithm>
#include <pb_decode.h>

namespace
{
    struct __attribute__((packed)) ConfigFooter
    {
        uint32_t dataSize;
        uint32_t dataCrc;
        uint32_t mainSize;
        uint32_t auxSize;
        uint32_t magic;
        uint32_t currentProfile;
    };

    constexpr uint32_t FOOTER_MAGIC = 0xd2f1e365;
    // The most data that fits in front of the footer
    constexpr uint32_t MAX_DATA_SIZE = EEPROM_SIZE_BYTES - sizeof(ConfigFooter);

    // Sizes come from the config tool or from flash, so they're added in 64 bits where they can't wrap
    bool sizes_fit(uint32_t data_size, uint32_t main_size, uint32_t aux_size)
    {
        return data_size <= MAX_DATA_SIZE && uint64_t(main_size) + aux_size <= data_size;
    }

    const ConfigFooter *footer_at(const uint8_t *end)
    {
        return reinterpret_cast<const ConfigFooter *>(end - sizeof(ConfigFooter));
    }

    bool read_image(const uint8_t *end, ConfigImage &image)
    {
        const ConfigFooter *footer = footer_at(end);
        if (footer->magic != FOOTER_MAGIC ||
            !sizes_fit(footer->dataSize, footer->mainSize, footer->auxSize) ||
            CRC32::calculate(end - sizeof(ConfigFooter) - footer->dataSize, footer->dataSize) != footer->dataCrc)
        {
            return false;
        }

        image.data = end - sizeof(ConfigFooter) - footer->dataSize;
        image.data_size = footer->dataSize;
        image.main_size = footer->mainSize;
        image.aux_size = footer->auxSize;
        image.current_profile = footer->currentProfile;
        return true;
    }
}

bool ConfigStorage::read_flash(ConfigImage &image, bool cached) const
{
    const uint8_t *start = cached
                               ? reinterpret_cast<const uint8_t *>(EEPROM.writeCache)
                               : reinterpret_cast<const uint8_t *>(EEPROM_ADDRESS_START);
    return read_image(start + EEPROM_SIZE_BYTES, image);
}

uint32_t ConfigStorage::read_chunk(uint8_t *buffer, uint32_t start, uint32_t max_size, bool cached) const
{
    ConfigImage image;
    if (!read_flash(image, cached))
    {
        return 0;
    }
    if (start >= image.data_size)
    {
        return 0;
    }
    const uint32_t remaining = image.data_size - start;
    const uint32_t size = remaining > max_size ? max_size : remaining;
    memcpy(buffer, image.data + start, size);
    return size;
}

bool ConfigStorage::initialize_empty() const
{
    ConfigFooter *footer = reinterpret_cast<ConfigFooter *>(
        EEPROM.writeCache + EEPROM_SIZE_BYTES - sizeof(ConfigFooter));
    footer->dataSize = 0;
    footer->mainSize = 0;
    footer->auxSize = 0;
    footer->dataCrc = CRC32::calculate(EEPROM.writeCache, 0);
    footer->magic = FOOTER_MAGIC;
    footer->currentProfile = 0;
    EEPROM.commit();
    return true;
}

ConfigMetadata ConfigStorage::read_metadata(bool cached) const
{
    const uint8_t *start = cached
                               ? reinterpret_cast<const uint8_t *>(EEPROM.writeCache)
                               : reinterpret_cast<const uint8_t *>(EEPROM_ADDRESS_START);
    const ConfigFooter *footer = footer_at(start + EEPROM_SIZE_BYTES);
    return {
        footer->dataSize,
        footer->dataCrc,
        footer->mainSize,
        footer->auxSize,
        footer->magic,
        footer->currentProfile};
}

bool ConfigStorage::write_info(const uint8_t *buffer, uint16_t bufsize)
{
    ConfigFooter *footer = reinterpret_cast<ConfigFooter *>(
        EEPROM.writeCache + EEPROM_SIZE_BYTES - sizeof(ConfigFooter));
    proto_ConfigInfo info proto_ConfigInfo_init_zero;
    pb_istream_t inputStream = pb_istream_from_buffer(buffer, bufsize);
    if (!pb_decode_delimited(&inputStream, proto_ConfigInfo_fields, &info))
    {
        return false;
    }
    // Leave the stored config alone unless the new one fits, adds up and is really a config
    if (info.dataSize < 0 || info.mainSize < 0 || info.auxSize < 0 ||
        uint32_t(info.magic) != FOOTER_MAGIC ||
        !sizes_fit(info.dataSize, info.mainSize, info.auxSize) ||
        uint64_t(info.mainSize) + uint64_t(info.auxSize) != uint64_t(info.dataSize))
    {
        return false;
    }
    footer->dataCrc = info.dataCrc;
    footer->dataSize = info.dataSize;
    footer->magic = info.magic;
    footer->mainSize = info.mainSize;
    footer->auxSize = info.auxSize;
    m_upload_active = info.dataSize != 0;
    return true;
}

void ConfigStorage::commit_after_write()
{
    m_should_commit = true;
}

ConfigStorage::WriteResult ConfigStorage::write_chunk(const uint8_t *buffer, uint16_t bufsize,
                                                      uint32_t start)
{
    const ConfigFooter &footer = *reinterpret_cast<const ConfigFooter *>(
        EEPROM.writeCache + EEPROM_SIZE_BYTES - sizeof(ConfigFooter));
    // Only chunks of an upload write_info accepted, and only inside the size it declared
    if (!m_upload_active || start >= footer.dataSize)
    {
        return WriteResult::Invalid;
    }
    const uint32_t length = std::min<uint32_t>(bufsize, footer.dataSize - start);
    memcpy(EEPROM.writeCache + start, buffer, length);
    if (start + length < footer.dataSize)
    {
        return WriteResult::InProgress;
    }
    m_upload_active = false;
    if (CRC32::calculate(EEPROM.writeCache, footer.dataSize) != footer.dataCrc)
    {
        return WriteResult::Invalid;
    }
    memmove(EEPROM.writeCache + EEPROM_SIZE_BYTES - sizeof(ConfigFooter) - footer.dataSize,
            EEPROM.writeCache, footer.dataSize);
    memset(EEPROM.writeCache, 0, EEPROM_SIZE_BYTES - sizeof(ConfigFooter) - footer.dataSize);
    return WriteResult::Done;
}

bool ConfigStorage::update_auxiliary(AuxiliaryWriter writer, void *context) const
{
    // Only re-sign a config that is valid as it stands, never a half finished or failed upload
    ConfigImage image;
    if (m_upload_active || !writer || !read_flash(image, true))
    {
        return false;
    }
    ConfigFooter *footer = reinterpret_cast<ConfigFooter *>(
        EEPROM.writeCache + EEPROM_SIZE_BYTES - sizeof(ConfigFooter));
    const uint32_t free_size = MAX_DATA_SIZE - image.data_size;
    const uint32_t main_size = image.main_size;

    // Build the new aux block in the free space in front of the data, so the stored config is untouched
    // if the writer fails
    uint32_t aux_size = 0;
    if (!writer(EEPROM.writeCache, free_size, aux_size, context) || aux_size > free_size)
    {
        memset(EEPROM.writeCache, 0, free_size);
        return false;
    }

    // [aux][free][main][old aux] -> [aux][main] -> [main][aux], then back against the footer
    memmove(EEPROM.writeCache + aux_size, EEPROM.writeCache + free_size, main_size);
    std::rotate(EEPROM.writeCache, EEPROM.writeCache + aux_size, EEPROM.writeCache + aux_size + main_size);

    footer->auxSize = aux_size;
    footer->dataSize = main_size + aux_size;
    footer->dataCrc = CRC32::calculate(EEPROM.writeCache, footer->dataSize);
    memmove(EEPROM.writeCache + EEPROM_SIZE_BYTES - sizeof(ConfigFooter) - footer->dataSize,
            EEPROM.writeCache, footer->dataSize);
    memset(EEPROM.writeCache, 0, EEPROM_SIZE_BYTES - sizeof(ConfigFooter) - footer->dataSize);
    EEPROM.commit();
    return true;
}
