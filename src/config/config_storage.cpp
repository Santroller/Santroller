#include "config/config_storage.hpp"

#include "CRC32.h"
#include "config/FlashPROM.h"
#include "config.pb.h"
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

const ConfigFooter *footer_at(const uint8_t *end)
{
    return reinterpret_cast<const ConfigFooter *>(end - sizeof(ConfigFooter));
}

bool read_image(const uint8_t *end, ConfigImage &image)
{
    const ConfigFooter *footer = footer_at(end);
    if (footer->magic != FOOTER_MAGIC ||
        footer->dataSize + sizeof(ConfigFooter) > EEPROM_SIZE_BYTES ||
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

bool ConfigStorage::read_cached(ConfigImage &image) const
{
    const uint8_t *start = reinterpret_cast<const uint8_t *>(EEPROM.writeCache);
    return read_image(start + EEPROM_SIZE_BYTES, image);
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

bool ConfigStorage::read_flash(ConfigImage &image) const
{
    const uint8_t *start = reinterpret_cast<const uint8_t *>(EEPROM_ADDRESS_START);
    return read_image(start + EEPROM_SIZE_BYTES, image);
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
        footer->currentProfile
    };
}

bool ConfigStorage::write_info(const uint8_t *buffer, uint16_t bufsize) const
{
    ConfigFooter *footer = reinterpret_cast<ConfigFooter *>(
        EEPROM.writeCache + EEPROM_SIZE_BYTES - sizeof(ConfigFooter));
    proto_ConfigInfo info proto_ConfigInfo_init_zero;
    pb_istream_t inputStream = pb_istream_from_buffer(buffer, bufsize);
    if (!pb_decode_delimited(&inputStream, proto_ConfigInfo_fields, &info))
    {
        return false;
    }
    footer->dataCrc = info.dataCrc;
    footer->dataSize = info.dataSize;
    footer->magic = info.magic;
    footer->mainSize = info.mainSize;
    footer->auxSize = info.auxSize;
    return true;
}

ConfigStorage::WriteResult ConfigStorage::write_chunk(const uint8_t *buffer, uint16_t bufsize,
                                                       uint32_t start) const
{
    const ConfigFooter &footer = *reinterpret_cast<const ConfigFooter *>(
        EEPROM.writeCache + EEPROM_SIZE_BYTES - sizeof(ConfigFooter));
    if (bufsize + start > footer.dataSize)
    {
        bufsize = footer.dataSize - start;
    }
    memcpy(EEPROM.writeCache + start, buffer, bufsize);
    if (start + bufsize < footer.dataSize)
    {
        return WriteResult::InProgress;
    }
    if (CRC32::calculate(EEPROM.writeCache, footer.dataSize) != footer.dataCrc)
    {
        return WriteResult::Invalid;
    }
    memmove(EEPROM.writeCache + EEPROM_SIZE_BYTES - sizeof(ConfigFooter) - footer.dataSize,
            EEPROM.writeCache, footer.dataSize);
    memset(EEPROM.writeCache, 0, EEPROM_SIZE_BYTES - sizeof(ConfigFooter) - footer.dataSize);
    EEPROM.commit();
    return WriteResult::Committed;
}

bool ConfigStorage::update_auxiliary(AuxiliaryWriter writer, void *context) const
{
    ConfigFooter *footer = reinterpret_cast<ConfigFooter *>(
        EEPROM.writeCache + EEPROM_SIZE_BYTES - sizeof(ConfigFooter));
    memmove(EEPROM.writeCache,
            EEPROM.writeCache + EEPROM_SIZE_BYTES - sizeof(ConfigFooter) - footer->dataSize,
            footer->dataSize);

    const uint32_t aux_capacity = EEPROM_SIZE_BYTES - footer->mainSize - sizeof(ConfigFooter);
    uint32_t aux_size = 0;
    if (!writer || !writer(EEPROM.writeCache + footer->mainSize,
                           aux_capacity, aux_size, context) || aux_size > aux_capacity)
    {
        return false;
    }

    footer->auxSize = aux_size;
    footer->dataSize = footer->mainSize + footer->auxSize;
    footer->dataCrc = CRC32::calculate(EEPROM.writeCache, footer->dataSize);
    memmove(EEPROM.writeCache + EEPROM_SIZE_BYTES - sizeof(ConfigFooter) - footer->dataSize,
            EEPROM.writeCache, footer->dataSize);
    memset(EEPROM.writeCache, 0, EEPROM_SIZE_BYTES - sizeof(ConfigFooter) - footer->dataSize);
    EEPROM.commit();
    return true;
}
