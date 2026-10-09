// The auxiliary block: state the device saves for itself (cycle and toggle inputs, bluetooth pairings and
// BTstack's TLV entries) after the main config. encode_auxiliary writes it, the decode_* callbacks read it.
#include <gtest/gtest.h>
#include <cstring>
#include <vector>
#include <pb_decode.h>
#include "config/aux_config.hpp"
#include "config/config_loader.hpp"
#include "config/config_storage.hpp"
#include "config_builder.hpp"
#include "config_test_support.hpp"

using namespace config_test;

namespace
{
class AuxConfigTest : public StorageTest
{
protected:
    void SetUp() override
    {
        StorageTest::SetUp();
        clear_state();
    }

    static void clear_state()
    {
        DeviceFactory::clear_cycle_states();
        DeviceFactory::clear_toggle_states();
        DeviceFactory::clear_bluetooth_pairing_states();
    }

    // Decode an aux block with the firmware's callbacks, like ConfigLoader::apply does
    static bool decode(const std::vector<uint8_t> &bytes)
    {
        proto_AuxConfigBlock block = proto_AuxConfigBlock_init_zero;
        block.states.funcs.decode = decode_cycle_input_states;
        block.toggleStates.funcs.decode = decode_toggle_input_states;
        block.bluetoothStates.funcs.decode = decode_bluetooth_states;
        block.tlvEntries.funcs.decode = decode_bluetooth_tlv_entries;
        pb_istream_t stream = pb_istream_from_buffer(bytes.data(), bytes.size());
        return pb_decode(&stream, proto_AuxConfigBlock_fields, &block);
    }

    static std::vector<uint8_t> encode_current_state(uint32_t capacity = 4096)
    {
        std::vector<uint8_t> buffer(capacity);
        uint32_t written = 0;
        EXPECT_TRUE(encode_auxiliary(buffer.data(), capacity, written, nullptr));
        buffer.resize(written);
        return buffer;
    }

    static size_t count_cycles()
    {
        size_t count = 0;
        DeviceFactory::foreach_cycle_state([&](int32_t, int32_t)
                                           { count++; });
        return count;
    }

    static size_t count_toggles()
    {
        size_t count = 0;
        DeviceFactory::foreach_toggle_state([&](int32_t, bool)
                                            { count++; });
        return count;
    }
};

// A length delimited field: tag byte, length, payload
std::vector<uint8_t> field(uint8_t tag, const std::vector<uint8_t> &payload)
{
    std::vector<uint8_t> out = {tag, uint8_t(payload.size())};
    out.insert(out.end(), payload.begin(), payload.end());
    return out;
}

std::vector<uint8_t> cat(std::vector<uint8_t> a, const std::vector<uint8_t> &b, const std::vector<uint8_t> &c,
                         const std::vector<uint8_t> &d)
{
    a.insert(a.end(), b.begin(), b.end());
    a.insert(a.end(), c.begin(), c.end());
    a.insert(a.end(), d.begin(), d.end());
    return a;
}

const uint8_t MAC[6] = {0xAA, 0xBB, 0xCC, 0x01, 0x02, 0x03};
const uint8_t LINK_KEY[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
}

TEST_F(AuxConfigTest, CycleAndToggleStatesDecode)
{
    AuxSpec aux;
    aux.cycles = {{1, 0}, {2, 3}, {100, -2}};
    aux.toggles = {{1, true}, {7, false}, {8, true}};
    ASSERT_TRUE(decode(encode_aux(aux)));
    EXPECT_EQ(DeviceFactory::get_cycle_state(1), 0);
    EXPECT_EQ(DeviceFactory::get_cycle_state(2), 3);
    EXPECT_EQ(DeviceFactory::get_cycle_state(100), -2);
    EXPECT_TRUE(DeviceFactory::get_toggle_state(1));
    EXPECT_FALSE(DeviceFactory::get_toggle_state(7));
    EXPECT_TRUE(DeviceFactory::get_toggle_state(8));
    EXPECT_EQ(count_cycles(), 3u);
    EXPECT_EQ(count_toggles(), 3u);
}

TEST_F(AuxConfigTest, ABluetoothPairingWithOnlyTheRequiredFieldsGetsTheDefaults)
{
    // Older firmware saved only id, mac, name and ble
    AuxSpec aux;
    aux.bluetooth.push_back(bluetooth_pairing(3, {0xAA, 0xBB, 0xCC, 1, 2, 3}, "Controller", false));
    ASSERT_TRUE(decode(encode_aux(aux)));
    DeviceFactory::BluetoothPairingStateData state{};
    ASSERT_TRUE(DeviceFactory::get_bluetooth_pairing_state(3, state));
    EXPECT_EQ(memcmp(state.mac, MAC, 6), 0);
    EXPECT_STREQ(state.name, "Controller");
    EXPECT_FALSE(state.ble);
    EXPECT_EQ(state.subtype, SubType_Gamepad);
    EXPECT_EQ(state.controller_type, BtControllerType_BtControllerTypeGeneric);
    EXPECT_EQ(state.vid, 0);
    EXPECT_EQ(state.pid, 0);
    EXPECT_FALSE(state.has_link_key);
}

TEST_F(AuxConfigTest, ABluetoothPairingWithEveryFieldDecodes)
{
    AuxSpec aux;
    auto pairing = bluetooth_pairing(0, {0xAA, 0xBB, 0xCC, 1, 2, 3}, "Guitar", true);
    pairing.has_subtype = true;
    pairing.subtype = RockBandGuitar;
    pairing.has_controllerType = true;
    pairing.controllerType = BtControllerType_BtControllerTypePS4;
    pairing.has_vid = true;
    pairing.vid = 0x054C;
    pairing.has_pid = true;
    pairing.pid = 0x09CC;
    pairing.has_linkKey = true;
    memcpy(pairing.linkKey, LINK_KEY, 16);
    aux.bluetooth.push_back(pairing);
    ASSERT_TRUE(decode(encode_aux(aux)));
    DeviceFactory::BluetoothPairingStateData state{};
    ASSERT_TRUE(DeviceFactory::get_bluetooth_pairing_state(0, state));
    EXPECT_TRUE(state.ble);
    EXPECT_EQ(state.subtype, RockBandGuitar);
    EXPECT_EQ(state.controller_type, BtControllerType_BtControllerTypePS4);
    EXPECT_EQ(state.vid, 0x054C);
    EXPECT_EQ(state.pid, 0x09CC);
    ASSERT_TRUE(state.has_link_key);
    EXPECT_EQ(memcmp(state.link_key, LINK_KEY, 16), 0);
}

TEST_F(AuxConfigTest, TlvEntriesDecode)
{
    AuxSpec aux;
    proto_BluetoothTlvEntry entry = proto_BluetoothTlvEntry_init_zero;
    entry.tag = 0x11223344;
    entry.value.size = 70;
    for (int i = 0; i < 70; i++)
    {
        entry.value.bytes[i] = uint8_t(i);
    }
    aux.tlv.push_back(entry);
    ASSERT_TRUE(decode(encode_aux(aux)));
    uint8_t value[80] = {};
    ASSERT_EQ(BtTlvStorage::instance().get_tag(0x11223344, value, sizeof(value)), 70);
    EXPECT_EQ(value[69], 69);
}

TEST_F(AuxConfigTest, WhatIsEncodedDecodesToTheSameState)
{
    DeviceFactory::set_cycle_state(4, 2);
    DeviceFactory::set_cycle_state(5, -7);
    DeviceFactory::set_toggle_state(6, true);
    DeviceFactory::set_toggle_state(9, false);
    DeviceFactory::set_bluetooth_pairing_state(2, MAC, "A name of exactly thirty-one ch", true, ProKeys,
                                               BtControllerType_BtControllerTypeSwitch, 0x057E, 0x2009, LINK_KEY);
    const uint8_t tlv[5] = {5, 4, 3, 2, 1};
    BtTlvStorage::instance().set_tag_from_config(0x99, tlv, sizeof(tlv));

    const auto bytes = encode_current_state();
    clear_state();
    ASSERT_TRUE(decode(bytes));

    EXPECT_EQ(DeviceFactory::get_cycle_state(4), 2);
    EXPECT_EQ(DeviceFactory::get_cycle_state(5), -7);
    EXPECT_TRUE(DeviceFactory::get_toggle_state(6));
    EXPECT_EQ(count_toggles(), 2u);
    DeviceFactory::BluetoothPairingStateData state{};
    ASSERT_TRUE(DeviceFactory::get_bluetooth_pairing_state(2, state));
    EXPECT_EQ(memcmp(state.mac, MAC, 6), 0);
    EXPECT_STREQ(state.name, "A name of exactly thirty-one ch");
    EXPECT_TRUE(state.ble);
    EXPECT_EQ(state.subtype, ProKeys);
    EXPECT_EQ(state.controller_type, BtControllerType_BtControllerTypeSwitch);
    EXPECT_EQ(state.vid, 0x057E);
    EXPECT_EQ(state.pid, 0x2009);
    ASSERT_TRUE(state.has_link_key);
    EXPECT_EQ(memcmp(state.link_key, LINK_KEY, 16), 0);
    uint8_t value[8] = {};
    ASSERT_EQ(BtTlvStorage::instance().get_tag(0x99, value, sizeof(value)), 5);
    EXPECT_EQ(value[0], 5);
}

// The encode_* callbacks used to ignore encoding failures, so an aux block that didn't fit was reported as
// written, cut short, and update_auxiliary signed and committed it
TEST_F(AuxConfigTest, EncodingFailsWhenItDoesNotFit)
{
    for (int i = 0; i < 20; i++)
    {
        DeviceFactory::set_cycle_state(i, i);
    }
    uint8_t buffer[16];
    uint32_t written = 0;
    EXPECT_FALSE(encode_auxiliary(buffer, sizeof(buffer), written, nullptr));
}

TEST_F(AuxConfigTest, SavedStateSurvivesAnAuxUpdateAndAReboot)
{
    // update_aux_cycle and friends: change the state, rewrite the aux block, commit
    ASSERT_EQ(upload(storage, encode_config({}), {}), ConfigStorage::WriteResult::Done);
    DeviceFactory::set_cycle_state(12, 3);
    DeviceFactory::set_toggle_state(13, true);
    DeviceFactory::set_bluetooth_pairing_state(0, MAC, "Pad", false);
    ASSERT_TRUE(storage.update_auxiliary(encode_auxiliary));
    run_pending_commit();

    clear_state();
    EEPROM.start();
    ConfigImage image;
    ASSERT_TRUE(storage.read_flash(image, true));
    ASSERT_TRUE(ConfigLoader::apply(image, ModeHid));
    EXPECT_EQ(DeviceFactory::get_cycle_state(12), 3);
    EXPECT_TRUE(DeviceFactory::get_toggle_state(13));
    DeviceFactory::BluetoothPairingStateData state{};
    ASSERT_TRUE(DeviceFactory::get_bluetooth_pairing_state(0, state));
    EXPECT_STREQ(state.name, "Pad");
}

TEST_F(AuxConfigTest, ATruncatedBluetoothPairingIsNotApplied)
{
    AuxSpec aux;
    aux.bluetooth.push_back(bluetooth_pairing(3, {1, 2, 3, 4, 5, 6}, "Controller", true));
    auto bytes = encode_aux(aux);
    // Keep the field header but cut the submessage short
    bytes.resize(bytes.size() - 4);
    EXPECT_FALSE(decode(bytes));
    DeviceFactory::BluetoothPairingStateData state{};
    EXPECT_FALSE(DeviceFactory::get_bluetooth_pairing_state(3, state));
}

TEST_F(AuxConfigTest, AnOversizedMacOrNameIsRejected)
{
    // macAddress is a fixed 6 bytes and name at most 31 characters plus the terminator
    const auto long_mac = field(0x1A, cat({0x08, 0x01}, field(0x12, std::vector<uint8_t>(7, 1)), field(0x1A, {}), {0x20, 0x00}));
    EXPECT_FALSE(decode(long_mac));
    const auto short_mac = field(0x1A, cat({0x08, 0x01}, field(0x12, std::vector<uint8_t>(5, 1)), field(0x1A, {}), {0x20, 0x00}));
    EXPECT_FALSE(decode(short_mac));
    const auto long_name = field(0x1A, cat({0x08, 0x01}, field(0x12, std::vector<uint8_t>(6, 1)), field(0x1A, std::vector<uint8_t>(32, 'x')), {0x20, 0x00}));
    EXPECT_FALSE(decode(long_name));
    DeviceFactory::BluetoothPairingStateData state{};
    EXPECT_FALSE(DeviceFactory::get_bluetooth_pairing_state(1, state));

    // and the same message with sizes that fit is fine
    const auto fits = field(0x1A, cat({0x08, 0x01}, field(0x12, std::vector<uint8_t>(6, 1)), field(0x1A, std::vector<uint8_t>(31, 'x')), {0x20, 0x00}));
    EXPECT_TRUE(decode(fits));
    EXPECT_TRUE(DeviceFactory::get_bluetooth_pairing_state(1, state));
}

TEST_F(AuxConfigTest, AnOversizedTlvValueIsRejected)
{
    // value is at most 70 bytes
    const auto bytes = field(0x22, cat({0x08, 0x01}, field(0x12, std::vector<uint8_t>(71, 0x55)), {}, {}));
    EXPECT_FALSE(decode(bytes));
    EXPECT_EQ(BtTlvStorage::instance().get_tag(1, nullptr, 0), 0);
}

// Like the bluetooth and TLV callbacks, the cycle and toggle ones only apply an entry that decoded, so a
// damaged aux block can't set an input's state to a value that was never saved
TEST_F(AuxConfigTest, ATruncatedCycleStateIsNotApplied)
{
    // CyclingInputState { id: 5, state: <missing> }
    const std::vector<uint8_t> bytes = {0x0A, 0x02, 0x08, 0x05};
    EXPECT_FALSE(decode(bytes));
    EXPECT_EQ(count_cycles(), 0u);
}

TEST_F(AuxConfigTest, ATruncatedToggleStateIsNotApplied)
{
    // ToggleInputState { id: 5, state: <missing> }
    const std::vector<uint8_t> bytes = {0x12, 0x02, 0x08, 0x05};
    EXPECT_FALSE(decode(bytes));
    EXPECT_EQ(count_toggles(), 0u);
}
