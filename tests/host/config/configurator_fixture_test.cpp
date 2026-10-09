// Configs encoded by the config tool's own protobufjs module (fixtures/encode_fixtures.mjs), uploaded and
// loaded the way the device does. Catches the tool's and the firmware's schemas, CRCs or upload framing
// drifting apart. The expected values are the ones encode_fixtures.mjs writes.
#include <gtest/gtest.h>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
#include <pb_decode.h>
#include "config/config_loader.hpp"
#include "config/config_storage.hpp"
#include "config/device_factory.hpp"
#include "config/profile_opts.hpp"
#include "devices/bt/bt_tlv_storage.hpp"
#include "config_test_support.hpp"

using namespace config_test;
using WriteResult = ConfigStorage::WriteResult;

namespace
{
std::vector<uint8_t> read_fixture(const std::string &name)
{
    std::ifstream file(std::string(SANTROLLER_CONFIG_FIXTURES_DIR) + "/" + name, std::ios::binary);
    EXPECT_TRUE(file.good()) << "missing fixture " << name;
    return std::vector<uint8_t>(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

// Decodes just the profiles' opts, the way config.cpp's load_opts does
struct OptsResult
{
    bool decoded = false;
    proto_ProfileOpts opts = proto_ProfileOpts_init_default;
};

bool decode_opts(pb_istream_t *stream, const pb_field_t *, void **arg)
{
    auto *result = static_cast<OptsResult *>(*arg);
    result->decoded = decode_profile_opts(stream, &result->opts);
    return result->decoded;
}

bool decode_profile_opts_only(pb_istream_t *stream, const pb_field_t *, void **arg)
{
    proto_Profile profile = proto_Profile_init_default;
    profile.opts.funcs.decode = decode_opts;
    profile.opts.arg = *arg;
    return pb_decode_ex(stream, proto_Profile_fields, &profile, PB_DECODE_NOINIT);
}

class ConfiguratorFixtureTest : public StorageTest
{
protected:
    void SetUp() override
    {
        StorageTest::SetUp();
        DeviceFactory::clear_cycle_states();
        DeviceFactory::clear_toggle_states();
        DeviceFactory::clear_bluetooth_pairing_states();
    }

    // Send the fixture as the tool does: the info report, then the data, each zero padded to 63 bytes
    WriteResult upload_fixture(const std::string &name)
    {
        auto info = read_fixture(name + ".info.bin");
        const auto data = read_fixture(name + ".data.bin");
        EXPECT_LE(info.size(), REPORT_SIZE);
        info.resize(REPORT_SIZE, 0);
        if (!storage.write_info(info.data(), uint16_t(info.size())))
        {
            return WriteResult::Invalid;
        }
        if (data.empty())
        {
            return WriteResult::Done;
        }
        WriteResult result = WriteResult::InProgress;
        uint32_t start = 0;
        for (const auto &report : data_reports(data))
        {
            result = storage.write_chunk(report.data(), REPORT_SIZE, start);
            start += REPORT_SIZE;
        }
        return result;
    }

    bool load_fixture(const std::string &name)
    {
        EXPECT_EQ(upload_fixture(name), WriteResult::Done);
        ConfigImage image;
        EXPECT_TRUE(storage.read_flash(image, true));
        return ConfigLoader::apply(image, ModeHid);
    }
};
}

TEST_F(ConfiguratorFixtureTest, TheToolsInfoReportDecodes)
{
    auto info = read_fixture("representative.info.bin");
    const auto data = read_fixture("representative.data.bin");
    info.resize(REPORT_SIZE, 0);
    ASSERT_TRUE(storage.write_info(info.data(), uint16_t(info.size())));
    const ConfigMetadata metadata = storage.read_metadata(true);
    EXPECT_EQ(metadata.data_size, data.size());
    // The magic is above INT32_MAX in an int32 field, which protobufjs writes as a 5 byte varint
    EXPECT_EQ(metadata.magic, CONFIG_MAGIC);
    EXPECT_EQ(metadata.main_size + metadata.aux_size, metadata.data_size);
}

TEST_F(ConfiguratorFixtureTest, TheToolsCrcMatchesTheFirmwares)
{
    EXPECT_EQ(upload_fixture("representative"), WriteResult::Done);
    EXPECT_EQ(upload_fixture("long_profile_name"), WriteResult::Done);
}

TEST_F(ConfiguratorFixtureTest, TheRepresentativeConfigsDevicesDecode)
{
    ASSERT_TRUE(load_fixture("representative"));
    const auto &devices = fake_loader::state.devices;
    ASSERT_EQ(devices.size(), 5u);

    EXPECT_EQ(devices[0].device.deviceid, 1);
    ASSERT_EQ(devices[0].device.which_device, proto_Device_usbHost_tag);
    const auto &usb = devices[0].device.device.usbHost;
    EXPECT_EQ(usb.firstPin, 20);
    EXPECT_TRUE(usb.enable5v);
    EXPECT_FALSE(usb.dmFirst);
    EXPECT_EQ(usb.mappingMode, PerInput);
    EXPECT_TRUE(usb.has_noteHoldTime);
    EXPECT_EQ(usb.noteHoldTime, 50u);

    EXPECT_EQ(devices[1].device.deviceid, 2);
    ASSERT_EQ(devices[1].device.which_device, proto_Device_ws2812_tag);
    EXPECT_EQ(devices[1].device.device.ws2812.pin, 23);
    EXPECT_EQ(devices[1].device.device.ws2812.type, Ws2812Grb);
    EXPECT_EQ(devices[1].device.device.ws2812.count, 5);

    EXPECT_EQ(devices[2].device.deviceid, 3);
    ASSERT_EQ(devices[2].device.which_device, proto_Device_cycle_tag);
    EXPECT_EQ(devices[2].device.device.cycle.type, custom);
    EXPECT_EQ(devices[2].cycle_values, (std::vector<int32_t>{0, 4, -1, 1000}));

    EXPECT_EQ(devices[3].device.deviceid, 4);
    ASSERT_EQ(devices[3].device.which_device, proto_Device_mpr121_tag);
    const auto &mpr121 = devices[3].device.device.mpr121;
    EXPECT_EQ(mpr121.i2c.block, 1);
    EXPECT_EQ(mpr121.i2c.sda, 18);
    EXPECT_EQ(mpr121.i2c.scl, 19);
    EXPECT_EQ(mpr121.i2c.clock, 400000);
    EXPECT_EQ(mpr121.touchpadCount, 12);

    EXPECT_EQ(devices[4].device.deviceid, 5);
    EXPECT_EQ(devices[4].device.which_device, proto_Device_toggle_tag);
}

TEST_F(ConfiguratorFixtureTest, TheRepresentativeConfigsProfilesDecode)
{
    ASSERT_TRUE(load_fixture("representative"));
    const auto &profiles = fake_loader::state.profiles;
    ASSERT_EQ(profiles.size(), 2u);

    const auto &guitar = profiles[0];
    ASSERT_TRUE(guitar.has_opts);
    EXPECT_EQ(guitar.opts.uid, 0x1234u);
    EXPECT_EQ(guitar.opts.faceButtonMappingMode, LegendBased);
    EXPECT_STREQ(guitar.opts.name, "Guitar");
    EXPECT_EQ(guitar.opts.deviceToEmulate, GuitarHeroGuitar);
    EXPECT_TRUE(guitar.opts.has_queueInputs && guitar.opts.queueInputs);
    EXPECT_TRUE(guitar.opts.has_dequeueInterval100us);
    EXPECT_EQ(guitar.opts.dequeueInterval100us, 20u);
    EXPECT_TRUE(guitar.opts.has_deviceSlotIdVersion);
    EXPECT_EQ(guitar.opts.deviceSlotIdVersion, 1u);
    EXPECT_TRUE(guitar.opts.has_ps3OnRpcs3);
    EXPECT_FALSE(guitar.opts.ps3OnRpcs3);
    EXPECT_FALSE(guitar.opts.has_xinputOnWindows);

    EXPECT_EQ(guitar.assignment_lists, 1u);
    ASSERT_EQ(guitar.assignments.size(), 2u);
    EXPECT_EQ(guitar.assignments[0].which_assignment, proto_ProfileAssignmentInfo_usbType_tag);
    EXPECT_EQ(guitar.assignments[0].assignment.usbType, GuitarHeroGuitar);
    ASSERT_EQ(guitar.assignments[1].which_assignment, proto_ProfileAssignmentInfo_consoleType_tag);
    const auto &console = guitar.assignments[1].assignment.consoleType;
    EXPECT_TRUE(console.has_consoleType);
    EXPECT_EQ(console.consoleType, ConsoleXbox360);
    EXPECT_TRUE(console.has_xinputOnWindows && console.xinputOnWindows);
    EXPECT_FALSE(console.has_forcedType);

    ASSERT_EQ(guitar.mappings.size(), 3u);
    const auto &a = guitar.mappings[0];
    ASSERT_EQ(a.mapping.which_mapping, proto_Output_gamepadButton_tag);
    EXPECT_EQ(a.mapping.mapping.gamepadButton, Gamepad_A);
    ASSERT_EQ(a.input.which_input, proto_Input_gpio_tag);
    EXPECT_EQ(a.input.input.gpio.pin, 2);
    EXPECT_EQ(a.input.input.gpio.pinMode, PullUp);
    EXPECT_FALSE(a.input.input.gpio.analog);
    EXPECT_TRUE(a.has_debounce);
    EXPECT_EQ(a.debounce, 5u);

    const auto &axis = guitar.mappings[1];
    ASSERT_EQ(axis.mapping.which_mapping, proto_Output_gamepadAxis_tag);
    EXPECT_EQ(axis.mapping.mapping.gamepadAxis, Gamepad_LeftStickX);
    EXPECT_EQ(axis.input.input.gpio.pin, 26);
    EXPECT_EQ(axis.input.input.gpio.pinMode, Floating);
    EXPECT_TRUE(axis.input.input.gpio.analog);
    EXPECT_TRUE(axis.has_min && axis.has_max && axis.has_center && axis.has_deadzone);
    EXPECT_EQ(axis.min, -32767);
    EXPECT_EQ(axis.max, 32767);
    EXPECT_EQ(axis.center, 0);
    EXPECT_EQ(axis.deadzone, 3000);

    const auto &fixed = guitar.mappings[2];
    EXPECT_EQ(fixed.mapping.mapping.gamepadButton, Gamepad_Start);
    ASSERT_EQ(fixed.input.which_input, proto_Input_fixed_tag);
    EXPECT_EQ(fixed.input.input.fixed.value, -5);

    ASSERT_EQ(guitar.leds.size(), 2u);
    ASSERT_EQ(guitar.leds[0].device.which_device, proto_LedDevice_gpio_tag);
    EXPECT_EQ(guitar.leds[0].device.device.gpio.pin, 25);
    EXPECT_EQ(guitar.leds[0].mapping.which_led, proto_LedMapping_staticMapping_tag);
    ASSERT_EQ(guitar.leds[1].device.which_device, proto_LedDevice_rgb_tag);
    const auto &rgb = guitar.leds[1].device.device.rgb;
    EXPECT_EQ(rgb.deviceId, 2);
    EXPECT_EQ(rgb.startR, 255);
    EXPECT_EQ(rgb.endB, 255);
    EXPECT_TRUE(rgb.hasStart);
    ASSERT_EQ(rgb.activeLed_count, 3);
    EXPECT_EQ(rgb.activeLed[0], 0);
    EXPECT_EQ(rgb.activeLed[1], 1);
    EXPECT_EQ(rgb.activeLed[2], 4);
    ASSERT_EQ(guitar.leds[1].mapping.which_led, proto_LedMapping_patternMapping_tag);
    EXPECT_EQ(guitar.leds[1].mapping.led.patternMapping.pattern, PatternFade);
    EXPECT_EQ(guitar.leds[1].mapping.led.patternMapping.speed, 3);
    EXPECT_EQ(guitar.leds[1].mapping.led.patternMapping.brightness, 200);

    const auto &pad = profiles[1];
    EXPECT_EQ(pad.opts.uid, 7u);
    EXPECT_EQ(pad.opts.faceButtonMappingMode, PositionBased);
    EXPECT_STREQ(pad.opts.name, "Pad");
    EXPECT_EQ(pad.opts.deviceToEmulate, Gamepad);
    EXPECT_FALSE(pad.opts.has_ps3OnRpcs3);
    EXPECT_EQ(pad.assignment_lists, 2u);
    ASSERT_EQ(pad.assignments.size(), 2u);
    EXPECT_EQ(pad.assignments[0].assignment.usbType, Gamepad);
    ASSERT_EQ(pad.assignments[1].which_assignment, proto_ProfileAssignmentInfo_bluetooth_tag);
    EXPECT_EQ(pad.assignments[1].assignment.bluetooth, BTStandard);
    ASSERT_EQ(pad.mappings.size(), 1u);
    EXPECT_EQ(pad.mappings[0].mapping.mapping.gamepadButton, Gamepad_X);
    ASSERT_EQ(pad.mappings[0].input.which_input, proto_Input_key_tag);
    EXPECT_EQ(pad.mappings[0].input.input.key.deviceid, 1);
    EXPECT_EQ(pad.mappings[0].input.input.key.key, 4);
    EXPECT_TRUE(pad.leds.empty());
}

TEST_F(ConfiguratorFixtureTest, TheRepresentativeConfigsTopLevelSettingsDecode)
{
    ASSERT_TRUE(load_fixture("representative"));
    ASSERT_TRUE(fake_loader::state.inactivity_had_config);
    const auto &inactivity = fake_loader::state.inactivity;
    EXPECT_EQ(inactivity.sleepTimeoutSec, 600u);
    EXPECT_EQ(inactivity.wakePin, 15);
    EXPECT_TRUE(inactivity.has_wakeActiveHigh);
    EXPECT_FALSE(inactivity.wakeActiveHigh);
    EXPECT_EQ(inactivity.ledTimeoutSec, 30u);
    EXPECT_EQ(fake_loader::state.secondary_pico_inits, 0);
}

TEST_F(ConfiguratorFixtureTest, TheRepresentativeConfigsAuxBlockDecodes)
{
    ASSERT_TRUE(load_fixture("representative"));
    EXPECT_EQ(DeviceFactory::get_cycle_state(3), 2);
    EXPECT_TRUE(DeviceFactory::get_toggle_state(5));

    DeviceFactory::BluetoothPairingStateData pairing{};
    ASSERT_TRUE(DeviceFactory::get_bluetooth_pairing_state(0, pairing));
    EXPECT_EQ(std::vector<uint8_t>(pairing.mac, pairing.mac + 6),
              (std::vector<uint8_t>{0xAA, 0xBB, 0xCC, 0x01, 0x02, 0x03}));
    EXPECT_STREQ(pairing.name, "Pad");
    EXPECT_TRUE(pairing.ble);
    EXPECT_EQ(pairing.subtype, RockBandGuitar);
    EXPECT_EQ(pairing.controller_type, BtControllerTypePS4);
    EXPECT_EQ(pairing.vid, 0x054C);
    EXPECT_EQ(pairing.pid, 0x09CC);
    ASSERT_TRUE(pairing.has_link_key);
    for (int i = 0; i < 16; i++)
    {
        EXPECT_EQ(pairing.link_key[i], i + 1);
    }

    uint8_t value[8] = {};
    ASSERT_EQ(BtTlvStorage::instance().get_tag(0x42544c00, value, sizeof(value)), 3);
    EXPECT_EQ(value[0], 7);
    EXPECT_EQ(value[1], 8);
    EXPECT_EQ(value[2], 9);
}

TEST_F(ConfiguratorFixtureTest, TheToolsEmptyConfigIsAValidEmptyImage)
{
    // The tool sends only the info report for an empty config
    ASSERT_TRUE(load_fixture("empty"));
    EXPECT_TRUE(fake_loader::state.devices.empty());
    EXPECT_TRUE(fake_loader::state.profiles.empty());
}

TEST_F(ConfiguratorFixtureTest, TheConfigReadsBackToTheToolByteForByte)
{
    // Like fetchConfigData in the tool
    ASSERT_EQ(upload_fixture("representative"), WriteResult::Done);
    std::vector<uint8_t> out;
    uint8_t buffer[REPORT_SIZE];
    uint32_t read;
    while ((read = storage.read_chunk(buffer, uint32_t(out.size()), REPORT_SIZE, true)) > 0)
    {
        out.insert(out.end(), buffer, buffer + read);
    }
    EXPECT_EQ(out, read_fixture("representative.data.bin"));
}

// Older config tools don't limit a profile's name, but ProfileOpts.name holds at most 31 bytes in the
// firmware (config.options: max_size 32). A longer name used to fail the whole ProfileOpts decode, leaving
// the profile without its uid, type or mappings. Now it is cut short and the rest of the profile loads.
TEST_F(ConfiguratorFixtureTest, AProfileNameLongerThanTheFirmwareHoldsStillLoads)
{
    const auto data = read_fixture("long_profile_name.data.bin");
    OptsResult result;
    proto_Config config = proto_Config_init_zero;
    config.profiles.funcs.decode = decode_profile_opts_only;
    config.profiles.arg = &result;
    pb_istream_t stream = pb_istream_from_buffer(data.data(), data.size());
    EXPECT_TRUE(pb_decode(&stream, proto_Config_fields, &config));

    ASSERT_TRUE(result.decoded);
    EXPECT_EQ(result.opts.uid, 1u);
    EXPECT_EQ(result.opts.deviceToEmulate, GuitarHeroGuitar);
    EXPECT_STREQ(result.opts.name, "My favourite guitar for Clone H");

    // and through the loader, with its mapping
    ASSERT_TRUE(load_fixture("long_profile_name"));
    ASSERT_EQ(fake_loader::state.profiles.size(), 1u);
    EXPECT_EQ(fake_loader::state.profiles[0].opts.uid, 1u);
    EXPECT_EQ(fake_loader::state.profiles[0].mappings.size(), 1u);
}
