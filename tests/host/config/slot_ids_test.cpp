// deviceSlotIdVersion 2 slot ids (ProfileSlotIds): a slot gets the same id in every assignment list
// of a profile, numbered per kind and subtype, so inputs always read the same type of device
// whichever list matched.
#include "config/slot_ids.hpp"
#include <gtest/gtest.h>
#include <vector>

namespace
{
using List = std::vector<proto_ProfileAssignmentInfo>;

proto_ProfileAssignmentInfo usb_type(SubType type)
{
    proto_ProfileAssignmentInfo info = proto_ProfileAssignmentInfo_init_default;
    info.which_assignment = proto_ProfileAssignmentInfo_usbType_tag;
    info.assignment.usbType = type;
    return info;
}

proto_ProfileAssignmentInfo usb_device(uint32_t vid, uint32_t pid, bool has_subtype, SubType subtype = Gamepad)
{
    proto_ProfileAssignmentInfo info = proto_ProfileAssignmentInfo_init_default;
    info.which_assignment = proto_ProfileAssignmentInfo_usbDevice_tag;
    info.assignment.usbDevice.vid = vid;
    info.assignment.usbDevice.pid = pid;
    info.assignment.usbDevice.has_subtype = has_subtype;
    info.assignment.usbDevice.subtype = subtype;
    return info;
}

proto_ProfileAssignmentInfo wii_ext(WiiExtType type)
{
    proto_ProfileAssignmentInfo info = proto_ProfileAssignmentInfo_init_default;
    info.which_assignment = proto_ProfileAssignmentInfo_wiiExt_tag;
    info.assignment.wiiExt = type;
    return info;
}

proto_ProfileAssignmentInfo midi_channel(uint32_t channel)
{
    proto_ProfileAssignmentInfo info = proto_ProfileAssignmentInfo_init_default;
    info.which_assignment = proto_ProfileAssignmentInfo_midiChannel_tag;
    info.assignment.midiChannel = channel;
    return info;
}

proto_ProfileAssignmentInfo midi_channel_from_usb(uint32_t channel, uint32_t vid, uint32_t pid)
{
    auto info = midi_channel(channel);
    info.has_midiDevice = true;
    info.midiDevice.which_device = proto_MidiSourceDevice_usbDevice_tag;
    info.midiDevice.device.usbDevice.vid = vid;
    info.midiDevice.device.usbDevice.pid = pid;
    return info;
}

proto_ProfileAssignmentInfo console()
{
    proto_ProfileAssignmentInfo info = proto_ProfileAssignmentInfo_init_default;
    info.which_assignment = proto_ProfileAssignmentInfo_consoleType_tag;
    return info;
}

// Scans the lists like the firmware does before loading a profile, then returns the id each
// assignment gets while loading
std::vector<std::vector<uint16_t>> slot_ids(const std::vector<List> &lists, uint32_t version = SLOT_ID_VERSION_LATEST)
{
    ProfileSlotIds ids;
    for (const auto &list : lists)
    {
        for (const auto &info : list)
        {
            ids.add(info);
        }
        ids.end_list();
    }
    ids.build(version);
    std::vector<std::vector<uint16_t>> result;
    for (const auto &list : lists)
    {
        ids.end_list();
        auto &out = result.emplace_back();
        for (const auto &info : list)
        {
            out.push_back(ids.next_id(info));
        }
    }
    return result;
}
} // namespace

TEST(SlotIdsTest, EachSubtypeKeepsItsIdInEveryList)
{
    auto gamepad = usb_type(Gamepad);
    auto keyboard = usb_type(KeyboardMouse);
    // A gamepad and keyboard together, or either on its own
    auto ids = slot_ids({{console(), gamepad, keyboard}, {console(), gamepad}, {console(), keyboard}, {console()}});
    EXPECT_EQ(ids, (std::vector<std::vector<uint16_t>>{{0, 1, 2}, {0, 1}, {0, 2}, {0}}));
}

TEST(SlotIdsTest, ListOrderDoesNotChangeTheIds)
{
    auto gamepad = usb_type(Gamepad);
    auto keyboard = usb_type(KeyboardMouse);
    auto ids = slot_ids({{keyboard}, {keyboard, gamepad}});
    EXPECT_EQ(ids, (std::vector<std::vector<uint16_t>>{{2}, {2, 1}}));
}

TEST(SlotIdsTest, SlotsOfTheSameSubtypeAreNumberedInListOrder)
{
    auto gamepad = usb_type(Gamepad);
    auto guitar = usb_type(GuitarHeroGuitar);
    auto ids = slot_ids({{gamepad, guitar, gamepad}, {gamepad}});
    EXPECT_EQ(ids, (std::vector<std::vector<uint16_t>>{{1, 3, 2}, {1}}));
}

TEST(SlotIdsTest, KindsAreNumberedSeparately)
{
    auto ids = slot_ids({{wii_ext(WiiClassicController), usb_type(Gamepad)}, {wii_ext(WiiClassicControllerPro)}});
    EXPECT_EQ(ids, (std::vector<std::vector<uint16_t>>{{1, 1}, {2}}));
}

TEST(SlotIdsTest, SpecificDevicesShareTheirSubtypesNumbering)
{
    // A specific gamepad is numbered with the other gamepads, one without a subtype after every
    // subtype
    auto ids = slot_ids({{usb_device(1, 2, false), usb_type(KeyboardMouse), usb_device(3, 4, true, Gamepad)},
                         {usb_type(Gamepad)}});
    EXPECT_EQ(ids, (std::vector<std::vector<uint16_t>>{{3, 2, 1}, {1}}));
}

TEST(SlotIdsTest, Version2SharesMidiSlotsAcrossChannelsAndDevices)
{
    // Profiles saved with version 2 keep the ids their inputs were set up with
    auto ids = slot_ids({{midi_channel(10)}, {midi_channel(5)}, {midi_channel_from_usb(10, 1, 2)}}, 2);
    EXPECT_EQ(ids, (std::vector<std::vector<uint16_t>>{{1}, {1}, {1}}));
}

TEST(SlotIdsTest, MidiSlotsOnDifferentChannelsOrDevicesAreDifferentSlots)
{
    auto ids = slot_ids({{midi_channel(10)}, {midi_channel(5)}, {midi_channel_from_usb(10, 1, 2)}, {midi_channel(10)}});
    EXPECT_EQ(ids, (std::vector<std::vector<uint16_t>>{{2}, {1}, {3}, {2}}));
}
