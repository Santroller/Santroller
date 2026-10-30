#pragma once
#include "config.pb.h"
#include "profiles/profile.hpp"
#include <cstdint>
#include <map>
#include <tuple>
#include <utility>

// Slots that don't have a subtype (MIDI channels, and specific devices that don't name one)
constexpr uint16_t SLOT_UNTYPED = 0xffff;

// The newest deviceSlotIdVersion
constexpr uint32_t SLOT_ID_VERSION_LATEST = 3;

// The slot kind and subtype of an assignment, or DeviceSlotKind::None for assignments that aren't
// slots
inline std::pair<DeviceSlotKind, uint16_t> slot_kind_and_type(const proto_ProfileAssignmentInfo &info)
{
    switch (info.which_assignment)
    {
    case proto_ProfileAssignmentInfo_wiiExt_tag:
        return {DeviceSlotKind::WiiExtension, static_cast<uint16_t>(info.assignment.wiiExt)};
    case proto_ProfileAssignmentInfo_ps2Cnt_tag:
        return {DeviceSlotKind::PS2, static_cast<uint16_t>(info.assignment.ps2Cnt)};
    case proto_ProfileAssignmentInfo_snesCnt_tag:
        return {DeviceSlotKind::SNES, static_cast<uint16_t>(info.assignment.snesCnt)};
    case proto_ProfileAssignmentInfo_joybusCnt_tag:
        return {DeviceSlotKind::Joybus, static_cast<uint16_t>(info.assignment.joybusCnt)};
    case proto_ProfileAssignmentInfo_usbType_tag:
        return {DeviceSlotKind::USB, static_cast<uint16_t>(info.assignment.usbType)};
    case proto_ProfileAssignmentInfo_usbDevice_tag:
        return {DeviceSlotKind::USB, info.assignment.usbDevice.has_subtype
                                         ? static_cast<uint16_t>(info.assignment.usbDevice.subtype)
                                         : SLOT_UNTYPED};
    case proto_ProfileAssignmentInfo_bluetoothType_tag:
        return {DeviceSlotKind::Bluetooth, static_cast<uint16_t>(info.assignment.bluetoothType)};
    case proto_ProfileAssignmentInfo_bluetoothDevice_tag:
        return {DeviceSlotKind::Bluetooth, info.assignment.bluetoothDevice.has_subtype
                                               ? static_cast<uint16_t>(info.assignment.bluetoothDevice.subtype)
                                               : SLOT_UNTYPED};
    case proto_ProfileAssignmentInfo_midiChannel_tag:
        return {DeviceSlotKind::MIDI, SLOT_UNTYPED};
    case proto_ProfileAssignmentInfo_midiProGuitar_tag:
        return {DeviceSlotKind::MIDI, static_cast<uint16_t>(info.assignment.midiProGuitar)};
    case proto_ProfileAssignmentInfo_xboxOneAuth_tag:
        return {DeviceSlotKind::USB, static_cast<uint16_t>(SubType_Gamepad)};
    default:
        return {DeviceSlotKind::None, SLOT_UNTYPED};
    }
}

// What tells slots of a kind apart for deviceSlotIdVersion. From version 3 a MIDI slot is also told
// apart by its channel and the device it's limited to, so slots on different channels or devices
// aren't read as the same one. Fits in 53 bits, so the configurator can mirror it.
inline uint64_t slot_key_type(const proto_ProfileAssignmentInfo &info, uint32_t version)
{
    auto [kind, type] = slot_kind_and_type(info);
    if (version < 3 || kind != DeviceSlotKind::MIDI)
    {
        return type;
    }
    uint64_t base = info.which_assignment == proto_ProfileAssignmentInfo_midiChannel_tag
                        ? 0x10000 + (info.assignment.midiChannel & 0xffff)
                        : type;
    uint64_t source = 0;
    uint64_t detail = 0;
    if (info.has_midiDevice)
    {
        const auto &device = info.midiDevice;
        switch (device.which_device)
        {
        case proto_MidiSourceDevice_usbHost_tag:
            source = 1;
            break;
        case proto_MidiSourceDevice_bluetooth_tag:
            source = 2;
            break;
        case proto_MidiSourceDevice_usbDevice_tag:
            source = 3;
            detail = ((device.device.usbDevice.vid & 0xffff) << 16) | (device.device.usbDevice.pid & 0xffff);
            break;
        case proto_MidiSourceDevice_bluetoothDevice_tag:
            source = 4;
            detail = ((device.device.bluetoothDevice.vid & 0xffff) << 16) | (device.device.bluetoothDevice.pid & 0xffff);
            break;
        case proto_MidiSourceDevice_deviceid_tag:
            source = 5;
            detail = static_cast<uint32_t>(device.device.deviceid);
            break;
        }
    }
    return (((source << 32) | detail) << 17) | base;
}

// Slot ids for deviceSlotIdVersion 2 and up. Each slot is the nth slot of its kind and type
// (slot_key_type) within an assignment list, and gets the same id in every list of the profile, so
// an input's deviceid always means the same type of device whichever list matched. Ids are
// numbered per kind, in order of type and then n, so they don't depend on the order of the lists.
// The version is only known once the profile has been scanned, so slots are numbered both ways
// until build() picks one.
class ProfileSlotIds
{
public:
    // Call for every assignment of a list, in order, then end_list() before the next list
    void add(const proto_ProfileAssignmentInfo &info)
    {
        m_v2.add(info, 2);
        m_latest.add(info, SLOT_ID_VERSION_LATEST);
    }
    void end_list()
    {
        m_v2.end_list();
        m_latest.end_list();
    }
    void build(uint32_t version = SLOT_ID_VERSION_LATEST)
    {
        m_version = version;
        numbering().build();
    }
    // The id of the next slot of a list being loaded, or 0 for assignments that aren't slots
    uint16_t next_id(const proto_ProfileAssignmentInfo &info)
    {
        return numbering().next_id(info, m_version);
    }

private:
    class Numbering
    {
    public:
        void add(const proto_ProfileAssignmentInfo &info, uint32_t version)
        {
            auto kind = slot_kind_and_type(info).first;
            if (kind != DeviceSlotKind::None)
            {
                auto type = slot_key_type(info, version);
                m_ids.emplace(Key{kind, type, ++m_list_counts[{kind, type}]}, 0);
            }
        }
        void end_list()
        {
            m_list_counts.clear();
        }
        void build()
        {
            std::map<DeviceSlotKind, uint16_t> next;
            // std::map orders by kind, then type, then n
            for (auto &[key, id] : m_ids)
            {
                id = ++next[std::get<0>(key)];
            }
        }
        uint16_t next_id(const proto_ProfileAssignmentInfo &info, uint32_t version)
        {
            auto kind = slot_kind_and_type(info).first;
            if (kind == DeviceSlotKind::None)
            {
                return 0;
            }
            auto type = slot_key_type(info, version);
            auto it = m_ids.find(Key{kind, type, ++m_list_counts[{kind, type}]});
            return it == m_ids.end() ? 0 : it->second;
        }

    private:
        using Key = std::tuple<DeviceSlotKind, uint64_t, uint16_t>;
        std::map<Key, uint16_t> m_ids;
        std::map<std::pair<DeviceSlotKind, uint64_t>, uint16_t> m_list_counts;
    };
    Numbering &numbering() { return m_version >= 3 ? m_latest : m_v2; }

    Numbering m_v2;
    Numbering m_latest;
    uint32_t m_version = SLOT_ID_VERSION_LATEST;
};
