#pragma once

#include <stdint.h>

struct ConfigImage
{
    const uint8_t *data;
    uint32_t data_size;
    uint32_t main_size;
    uint32_t aux_size;
    uint32_t current_profile;
};

struct ConfigMetadata
{
    uint32_t data_size;
    uint32_t data_crc;
    uint32_t main_size;
    uint32_t aux_size;
    uint32_t magic;
    uint32_t current_profile;
};

class ConfigStorage
{
public:
    using AuxiliaryWriter = bool (*)(uint8_t *buffer, uint32_t capacity, uint32_t &written, void *context);

    enum class WriteResult
    {
        InProgress,
        Committed,
        Invalid
    };

    bool initialize_empty() const;
    bool read_cached(ConfigImage &image) const;
    bool read_flash(ConfigImage &image) const;
    ConfigMetadata read_metadata(bool cached) const;
    bool write_info(const uint8_t *buffer, uint16_t bufsize) const;
    WriteResult write_chunk(const uint8_t *buffer, uint16_t bufsize, uint32_t start) const;
    bool update_auxiliary(AuxiliaryWriter writer, void *context = nullptr) const;
};
