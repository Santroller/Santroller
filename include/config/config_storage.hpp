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
        Done,
        Invalid
    };

    // Run from an empty config held in the cache only. Flash keeps whatever it had (eg a config this
    // firmware couldn't load) until the tool writes a real one or saves, see preserving_flash
    bool initialize_empty();
    // Set by initialize_empty, and by an upload that differs from flash: nothing but an explicit save
    // (or an upload matching flash) may commit the cache to flash
    bool preserving_flash() const { return m_preserve_flash; }
    void stop_preserving_flash() { m_preserve_flash = false; }
    bool read_flash(ConfigImage &image, bool cached) const;
    // Copy up to max_size bytes of the stored image, starting start bytes in. Returns how many were copied
    uint32_t read_chunk(uint8_t *buffer, uint32_t start, uint32_t max_size, bool cached) const;
    void commit_after_write();
    ConfigMetadata read_metadata(bool cached) const;
    bool write_info(const uint8_t *buffer, uint16_t bufsize);
    WriteResult write_chunk(const uint8_t *buffer, uint16_t bufsize, uint32_t start);
    bool upload_active() const { return m_upload_active; }
    // Throw away an upload that failed or was never finished, putting back the config it was replacing
    void abort_write();
    // Rewrite the aux block from writer, committing it to flash unless commit is false
    bool update_auxiliary(AuxiliaryWriter writer, void *context = nullptr, bool commit = true) const;
private:
    bool m_should_commit = false;
    bool m_preserve_flash = false;
    // Set while write_info's upload is waiting for its data
    bool m_upload_active = false;
    // write_info replaces the cached footer, this is the one it replaced
    uint8_t m_previous_footer[24];
    bool m_have_previous_footer = false;
};
