#pragma once
// Ids for Device::hardware_id(), which emulated pads take their serials from
#include <stdint.h>
#include <optional>

// MAC addresses take the low 48 bits; the 32 bit serials controllers report sit above them (1 << 48), and USB
// iSerialNumber hashes above those (2 << 48, see UsbHostInterface::hardware_id), so none of them can collide
inline std::optional<uint64_t> hardware_id_from_mac(const uint8_t mac[6])
{
    uint64_t id = 0;
    for (int i = 0; i < 6; i++)
    {
        id = (id << 8) | mac[i];
    }
    return id ? std::optional<uint64_t>(id) : std::nullopt;
}

inline std::optional<uint64_t> hardware_id_from_serial(uint32_t serial)
{
    // all zeros or all ones is a controller that doesn't really have one
    if (serial == 0 || serial == 0xFFFFFFFF)
    {
        return std::nullopt;
    }
    return (1ull << 48) | serial;
}
