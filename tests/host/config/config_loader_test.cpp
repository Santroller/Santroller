// ConfigLoader::apply: decodes a stored image. The main section is a Config whose repeated devices and
// profiles go to config.cpp's load_device / load_profile (here, fakes that record what they were given),
// and the aux section is an AuxConfigBlock whose entries go to the decode_* callbacks in aux_config.hpp.
#include <gtest/gtest.h>
#include <cstring>
#include <string>
#include <vector>
#include "config/config_loader.hpp"
#include "config/config_storage.hpp"
#include "config/device_factory.hpp"
#include "devices/bt/bt_tlv_storage.hpp"
#include "config_builder.hpp"
#include "config_test_support.hpp"

using namespace config_test;
using WriteResult = ConfigStorage::WriteResult;

namespace
{
ConfigSpec realistic_config()
{
    ConfigSpec spec;
    spec.devices.push_back(usb_host_device(1, 20));
    spec.devices.push_back(ws2812_device(2, 23, 5));
    spec.devices.push_back(cycle_device(3, {0, 4, -1, 1000}));

    ProfileSpec guitar;
    guitar.opts = profile_opts(0x1234, "Guitar", GuitarHeroGuitar);
    guitar.opts.has_queueInputs = true;
    guitar.opts.queueInputs = true;
    guitar.assignments.push_back({usb_type_assignment(GuitarHeroGuitar)});
    guitar.mappings.push_back(gpio_button(2, Gamepad_A));
    guitar.mappings.push_back(gpio_button(3, Gamepad_B));
    guitar.mappings.push_back(gpio_button(4, Gamepad_Start));
    guitar.leds.push_back(gpio_static_led(25));
    spec.profiles.push_back(guitar);

    ProfileSpec pad;
    pad.opts = profile_opts(7, "Pad", Gamepad);
    pad.assignments.push_back({usb_type_assignment(Gamepad), usb_type_assignment(FightStick)});
    pad.assignments.push_back({});
    pad.mappings.push_back(gpio_button(10, Gamepad_X));
    spec.profiles.push_back(pad);

    proto_InactivityConfig inactivity = proto_InactivityConfig_init_zero;
    inactivity.has_sleepTimeoutSec = true;
    inactivity.sleepTimeoutSec = 600;
    inactivity.has_wakePin = true;
    inactivity.wakePin = 15;
    spec.inactivity = inactivity;
    return spec;
}

class ConfigLoaderTest : public StorageTest
{
protected:
    void SetUp() override
    {
        StorageTest::SetUp();
        DeviceFactory::clear_cycle_states();
        DeviceFactory::clear_toggle_states();
        DeviceFactory::clear_bluetooth_pairing_states();
    }

    // Store main + aux the way the tool uploads them, then load the cached image like load() does
    bool store_and_apply(const std::vector<uint8_t> &main, const std::vector<uint8_t> &aux,
                         ConsoleMode current_mode = ModeHid)
    {
        EXPECT_EQ(upload(storage, main, aux), WriteResult::Done);
        ConfigImage image;
        EXPECT_TRUE(storage.read_flash(image, true));
        return ConfigLoader::apply(image, current_mode);
    }

    // Load an image straight from flash, bypassing the upload's CRC handling
    bool apply_raw(const std::vector<uint8_t> &main, const std::vector<uint8_t> &aux)
    {
        boot_with_flash_image(main, aux);
        ConfigImage image;
        EXPECT_TRUE(storage.read_flash(image, true));
        return ConfigLoader::apply(image, ModeHid);
    }
};
}

TEST_F(ConfigLoaderTest, ARealisticConfigRoundTrips)
{
    ASSERT_TRUE(store_and_apply(encode_config(realistic_config()), {}));
    const auto &state = fake_loader::state;

    ASSERT_EQ(state.devices.size(), 3u);
    EXPECT_EQ(state.devices[0].device.deviceid, 1);
    ASSERT_EQ(state.devices[0].device.which_device, proto_Device_usbHost_tag);
    EXPECT_EQ(state.devices[0].device.device.usbHost.firstPin, 20);
    EXPECT_TRUE(state.devices[0].device.device.usbHost.enable5v);
    EXPECT_EQ(state.devices[0].device.device.usbHost.mappingMode, PerInput);
    EXPECT_EQ(state.devices[1].device.deviceid, 2);
    ASSERT_EQ(state.devices[1].device.which_device, proto_Device_ws2812_tag);
    EXPECT_EQ(state.devices[1].device.device.ws2812.pin, 23);
    EXPECT_EQ(state.devices[1].device.device.ws2812.type, Ws2812Grb);
    EXPECT_EQ(state.devices[1].device.device.ws2812.count, 5);
    ASSERT_EQ(state.devices[2].device.which_device, proto_Device_cycle_tag);
    EXPECT_EQ(state.devices[2].cycle_values, (std::vector<int32_t>{0, 4, -1, 1000}));

    ASSERT_EQ(state.profiles.size(), 2u);
    const auto &guitar = state.profiles[0];
    ASSERT_TRUE(guitar.has_opts);
    EXPECT_EQ(guitar.opts.uid, 0x1234u);
    EXPECT_STREQ(guitar.opts.name, "Guitar");
    EXPECT_EQ(guitar.opts.deviceToEmulate, GuitarHeroGuitar);
    EXPECT_TRUE(guitar.opts.has_queueInputs && guitar.opts.queueInputs);
    EXPECT_FALSE(guitar.opts.has_ps3OnRpcs3);
    EXPECT_EQ(guitar.assignment_lists, 1u);
    ASSERT_EQ(guitar.assignments.size(), 1u);
    EXPECT_EQ(guitar.assignments[0].which_assignment, proto_ProfileAssignmentInfo_usbType_tag);
    EXPECT_EQ(guitar.assignments[0].assignment.usbType, GuitarHeroGuitar);
    ASSERT_EQ(guitar.mappings.size(), 3u);
    EXPECT_EQ(guitar.mappings[0].mapping.which_mapping, proto_Output_gamepadButton_tag);
    EXPECT_EQ(guitar.mappings[0].mapping.mapping.gamepadButton, Gamepad_A);
    EXPECT_EQ(guitar.mappings[0].input.which_input, proto_Input_gpio_tag);
    EXPECT_EQ(guitar.mappings[0].input.input.gpio.pin, 2);
    EXPECT_EQ(guitar.mappings[0].input.input.gpio.pinMode, PullUp);
    EXPECT_EQ(guitar.mappings[2].mapping.mapping.gamepadButton, Gamepad_Start);
    ASSERT_EQ(guitar.leds.size(), 1u);
    EXPECT_EQ(guitar.leds[0].device.which_device, proto_LedDevice_gpio_tag);
    EXPECT_EQ(guitar.leds[0].device.device.gpio.pin, 25);
    EXPECT_EQ(guitar.leds[0].mapping.which_led, proto_LedMapping_staticMapping_tag);

    const auto &pad = state.profiles[1];
    EXPECT_EQ(pad.opts.uid, 7u);
    EXPECT_STREQ(pad.opts.name, "Pad");
    EXPECT_EQ(pad.assignment_lists, 2u);
    ASSERT_EQ(pad.assignments.size(), 2u);
    EXPECT_EQ(pad.assignments[1].assignment.usbType, FightStick);
    ASSERT_EQ(pad.mappings.size(), 1u);
    EXPECT_EQ(pad.mappings[0].mapping.mapping.gamepadButton, Gamepad_X);
    EXPECT_TRUE(pad.leds.empty());

    EXPECT_TRUE(state.inactivity_configured);
    ASSERT_TRUE(state.inactivity_had_config);
    EXPECT_EQ(state.inactivity.sleepTimeoutSec, 600u);
    EXPECT_EQ(state.inactivity.wakePin, 15);
    EXPECT_EQ(state.secondary_pico_inits, 0);
}

TEST_F(ConfigLoaderTest, AnEmptyImageLoadsAsAnEmptyConfig)
{
    ASSERT_TRUE(storage.initialize_empty());
    ConfigImage image;
    ASSERT_TRUE(storage.read_flash(image, true));
    EXPECT_TRUE(ConfigLoader::apply(image, ModeHid));
    EXPECT_TRUE(fake_loader::state.devices.empty());
    EXPECT_TRUE(fake_loader::state.profiles.empty());
    EXPECT_TRUE(fake_loader::state.inactivity_configured);
    EXPECT_FALSE(fake_loader::state.inactivity_had_config);
}

TEST_F(ConfigLoaderTest, TheMainSectionStopsWhereTheAuxSectionStarts)
{
    // AuxConfigBlock.toggleStates is field 2, the same as Config.devices: decoding past main_size would
    // turn the toggle states into devices
    ConfigSpec spec;
    spec.devices.push_back(usb_host_device(1, 20));
    AuxSpec aux;
    aux.toggles = {{5, true}, {6, false}};
    ASSERT_TRUE(store_and_apply(encode_config(spec), encode_aux(aux)));
    EXPECT_EQ(fake_loader::state.devices.size(), 1u);
    EXPECT_TRUE(DeviceFactory::get_toggle_state(5));
}

TEST_F(ConfigLoaderTest, TheAuxSectionIsApplied)
{
    AuxSpec aux;
    aux.cycles = {{3, 2}, {9, -4}};
    aux.toggles = {{4, true}};
    aux.bluetooth.push_back(bluetooth_pairing(0, {1, 2, 3, 4, 5, 6}, "Pad", true));
    proto_BluetoothTlvEntry tlv = proto_BluetoothTlvEntry_init_zero;
    tlv.tag = 0x42544c00;
    tlv.value.size = 3;
    tlv.value.bytes[0] = 7;
    tlv.value.bytes[1] = 8;
    tlv.value.bytes[2] = 9;
    aux.tlv.push_back(tlv);
    ASSERT_TRUE(store_and_apply(encode_config(realistic_config()), encode_aux(aux)));

    EXPECT_EQ(DeviceFactory::get_cycle_state(3), 2);
    EXPECT_EQ(DeviceFactory::get_cycle_state(9), -4);
    EXPECT_TRUE(DeviceFactory::get_toggle_state(4));
    DeviceFactory::BluetoothPairingStateData pairing{};
    ASSERT_TRUE(DeviceFactory::get_bluetooth_pairing_state(0, pairing));
    EXPECT_EQ(std::vector<uint8_t>(pairing.mac, pairing.mac + 6), (std::vector<uint8_t>{1, 2, 3, 4, 5, 6}));
    EXPECT_STREQ(pairing.name, "Pad");
    uint8_t value[8] = {};
    ASSERT_EQ(BtTlvStorage::instance().get_tag(0x42544c00, value, sizeof(value)), 3);
    EXPECT_EQ(value[2], 9);
    // and the main config alongside it
    EXPECT_EQ(fake_loader::state.devices.size(), 3u);
    EXPECT_EQ(fake_loader::state.profiles.size(), 2u);
}

TEST_F(ConfigLoaderTest, StateFromThePreviousConfigIsClearedOnReload)
{
    AuxSpec aux;
    aux.cycles = {{3, 2}};
    aux.toggles = {{4, true}};
    aux.bluetooth.push_back(bluetooth_pairing(1, {9, 9, 9, 9, 9, 9}, "Old", false));
    ASSERT_TRUE(store_and_apply(encode_config({}), encode_aux(aux)));
    ASSERT_EQ(DeviceFactory::get_cycle_state(3), 2);

    ASSERT_TRUE(store_and_apply(encode_config({}), {}));
    EXPECT_EQ(DeviceFactory::get_cycle_state(3), 0);
    EXPECT_FALSE(DeviceFactory::get_toggle_state(4));
    DeviceFactory::BluetoothPairingStateData pairing{};
    EXPECT_FALSE(DeviceFactory::get_bluetooth_pairing_state(1, pairing));
}

TEST_F(ConfigLoaderTest, ACorruptAuxSectionStillLoadsTheMainConfig)
{
    // The aux block is the device's own runtime state; losing it shouldn't lose the config
    const std::vector<uint8_t> aux = {0x0A, 0x50, 0x08};
    EXPECT_TRUE(store_and_apply(encode_config(realistic_config()), aux));
    EXPECT_EQ(fake_loader::state.devices.size(), 3u);
    EXPECT_EQ(fake_loader::state.profiles.size(), 2u);
}

TEST_F(ConfigLoaderTest, ATruncatedMainSectionFailsToLoad)
{
    const auto main = encode_config(realistic_config());
    for (size_t cut : {size_t(1), main.size() / 2, main.size() - 1})
    {
        SCOPED_TRACE(cut);
        fake_loader::reset();
        EXPECT_FALSE(apply_raw(std::vector<uint8_t>(main.begin(), main.begin() + cut), {}));
    }
}

TEST_F(ConfigLoaderTest, GarbageFailsToLoadWithoutReadingPastTheImage)
{
    for (uint8_t seed : {1, 2, 3, 0x80, 0xFF})
    {
        SCOPED_TRACE(int(seed));
        fake_loader::reset();
        // ASan catches any read outside the image
        apply_raw(pattern(64, seed), pattern(16, uint8_t(seed + 1)));
    }
    fake_loader::reset();
    EXPECT_FALSE(apply_raw(std::vector<uint8_t>(100, 0xFF), {}));
}

TEST_F(ConfigLoaderTest, PeripheralBootStartsTheSecondaryPico)
{
    ConfigSpec spec;
    proto_PeripheralBootConfig boot = proto_PeripheralBootConfig_init_zero;
    boot.i2c.block = 1;
    boot.i2c.sda = 18;
    boot.i2c.scl = 19;
    boot.idPin = 22;
    spec.peripheral_boot = boot;
    ASSERT_TRUE(store_and_apply(encode_config(spec), {}));
    EXPECT_EQ(fake_loader::state.secondary_pico_inits, 1);
}

TEST_F(ConfigLoaderTest, TheConfigInterfaceIsAddedWhenNothingElseIsActive)
{
    fake_loader::state.has_active_instances = false;
    fake_loader::state.requested_mode = ModePs3;
    ASSERT_TRUE(store_and_apply(encode_config({}), {}, ModePs3));
    EXPECT_EQ(fake_loader::state.usb_instances_added, 1);
}

TEST_F(ConfigLoaderTest, Xbox360ModeAddsTheConfigAndSecurityInterfaces)
{
    fake_loader::state.requested_mode = ModeXbox360;
    ASSERT_TRUE(store_and_apply(encode_config({}), {}, ModeXbox360));
    EXPECT_EQ(fake_loader::state.usb_instances_added, 2);
    EXPECT_EQ(fake_loader::state.reinitialize_device_stack_calls, 0);
}

TEST_F(ConfigLoaderTest, AModeChangeReinitialisesTheDeviceStack)
{
    fake_loader::state.requested_mode = ModePs4;
    ASSERT_TRUE(store_and_apply(encode_config({}), {}, ModeHid));
    EXPECT_EQ(fake_loader::state.reinitialize_device_stack_calls, 1);
}

TEST_F(ConfigLoaderTest, AnUnchangedModeDoesNotReinitialiseTheDeviceStack)
{
    fake_loader::state.requested_mode = ModePs4;
    ASSERT_TRUE(store_and_apply(encode_config({}), {}, ModePs4));
    EXPECT_EQ(fake_loader::state.reinitialize_device_stack_calls, 0);
}

TEST_F(ConfigLoaderTest, ABatteryAppearingReinitialisesTheDeviceStack)
{
    fake_loader::state.requested_mode = ModePs4;
    fake_loader::state.battery_present_changed = true;
    ASSERT_TRUE(store_and_apply(encode_config({}), {}, ModePs4));
    EXPECT_EQ(fake_loader::state.reinitialize_device_stack_calls, 1);
}

TEST_F(ConfigLoaderTest, DevicesAreResetBeforeDecodingAndPrunedAfter)
{
    ASSERT_TRUE(store_and_apply(encode_config(realistic_config()), {}));
    const auto &calls = fake_loader::state.calls;
    auto index = [&](const std::string &name)
    {
        for (size_t i = 0; i < calls.size(); i++)
        {
            if (calls[i] == name)
            {
                return int(i);
            }
        }
        return -1;
    };
    ASSERT_GE(index("mark_root_devices_disconnected"), 0);
    ASSERT_GE(index("remove_disconnected_root_devices"), 0);
    ASSERT_GE(index("load_device"), 0);
    ASSERT_GE(index("load_profile"), 0);
    EXPECT_LT(index("mark_root_devices_disconnected"), index("load_device"));
    EXPECT_LT(index("clear_assignable_devices"), index("load_device"));
    EXPECT_LT(index("prepare_for_config_reload"), index("load_profile"));
    EXPECT_LT(index("load_device"), index("load_profile"));
    EXPECT_LT(index("load_profile"), index("remove_disconnected_root_devices"));
    EXPECT_LT(index("clear_active_devices"), index("remove_disconnected_root_devices"));
    EXPECT_LT(index("prepare_for_config_reload"), index("finish_config_reload"));
    EXPECT_LT(index("mark_root_devices_disconnected"), index("remove_disconnected_root_devices"));
    EXPECT_EQ(calls.back(), "sync_requested_mode_to_current");
}
