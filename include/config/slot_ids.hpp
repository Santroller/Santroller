#pragma once
#include "config.pb.h"
#include "profiles/profile.hpp"
#include <cstdint>
#include <map>
#include <tuple>
#include <utility>

// Slots that don't have a subtype (MIDI channels, and specific devices that don't name one)
constexpr uint16_t SLOT_UNTYPED = 0xffff;

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

// Slot ids for deviceSlotIdVersion 2. Each slot is the nth slot of its kind and subtype within an
// assignment list, and gets the same id in every list of the profile, so an input's deviceid
// always means the same type of device whichever list matched. Ids are numbered per kind, in
// order of subtype and then n, so they don't depend on the order of the lists.
class ProfileSlotIds
{
public:
    // Call for every assignment of a list, in order, then end_list() before the next list
    void add(const proto_ProfileAssignmentInfo &info)
    {
        auto [kind, type] = slot_kind_and_type(info);
        if (kind != DeviceSlotKind::None)
        {
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
        // std::map orders by kind, then subtype, then n
        for (auto &[key, id] : m_ids)
        {
            id = ++next[std::get<0>(key)];
        }
    }
    // The id of the next slot of a list being loaded, or 0 for assignments that aren't slots
    uint16_t next_id(const proto_ProfileAssignmentInfo &info)
    {
        auto [kind, type] = slot_kind_and_type(info);
        if (kind == DeviceSlotKind::None)
        {
            return 0;
        }
        auto it = m_ids.find(Key{kind, type, ++m_list_counts[{kind, type}]});
        return it == m_ids.end() ? 0 : it->second;
    }

private:
    using Key = std::tuple<DeviceSlotKind, uint16_t, uint16_t>;
    std::map<Key, uint16_t> m_ids;
    std::map<std::pair<DeviceSlotKind, uint16_t>, uint16_t> m_list_counts;
};
