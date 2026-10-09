// Device and mode based profile activation (assignmentTypeDesc in the configurator):
//  - wiiExt / ps2Cnt / usbType / usbDevice / bluetoothType / midiChannel: "Match when a ... is
//    plugged into this device" - the profile claims the matching connected controller.
//  - consoleType: "Match when this device plugged into another via USB", optionally only for a
//    "specific console" (specificConsoleDesc), optionally forcing a mode (forcedTypeDesc).
//  - bluetooth: "Match when emulating a bluetooth device".
//  - ps2Emulation / wiiEmulation: "Match when plugged into a PS1/PS2 via the PS controller port" /
//    "into a wii remote via the wii extension port".
#include "inputs_test_support.hpp"
#include "triggers/device_type_triggers.hpp"
#include "triggers/mode_triggers.hpp"
#include <algorithm>

namespace
{
class DeviceTriggerTest : public InputsTest
{
protected:
    std::shared_ptr<Profile> profile = make_profile();

    std::shared_ptr<TestDevice> plug_in(uint16_t id)
    {
        auto device = std::make_shared<TestDevice>(id);
        DeviceManager::instance().add_assignable_device(device);
        return device;
    }
};

proto_UsbDeviceAssignment usb_assignment()
{
    proto_UsbDeviceAssignment config;
    memset(&config, 0, sizeof(config));
    return config;
}
} // namespace

TEST_F(DeviceTriggerTest, WiiExtensionMatchesOnlyThatExtension)
{
    WiiExtTypeActivationTrigger trigger(WiiGuitarHeroGuitar, profile, 1, 0, 0);
    EXPECT_FALSE(trigger.validate(false, false, false));
    auto nunchuk = plug_in(10);
    nunchuk->wii_type = WiiNunchuk;
    EXPECT_FALSE(trigger.validate(false, false, false));
    auto guitar = plug_in(11);
    guitar->wii_type = WiiGuitarHeroGuitar;
    EXPECT_TRUE(trigger.validate(false, false, false));
    // Checking doesn't take it
    EXPECT_EQ(DeviceManager::instance().assignable_device_count(), 2u);
    EXPECT_TRUE(profile->claimed_devices.empty());
}

TEST_F(DeviceTriggerTest, ClaimingTakesTheDeviceIntoItsSlot)
{
    auto instance = std::make_shared<Instance>();
    instance->player_led = 3;
    ProfileManager::instance().instances[profile->profile_id] = {instance};
    auto guitar = plug_in(11);
    guitar->wii_type = WiiGuitarHeroGuitar;
    WiiExtTypeActivationTrigger trigger(WiiGuitarHeroGuitar, profile, 1, 2, 0);
    EXPECT_TRUE(trigger.validate(true, false, false));
    EXPECT_EQ(DeviceManager::instance().assignable_device_count(), 0u);
    EXPECT_EQ(profile->get_claimed_device(DeviceSlotKind::WiiExtension, 2), guitar);
    // The new controller picks up the player LED etc of what it's now part of
    EXPECT_EQ(guitar->player_led, 3);
    EXPECT_EQ(instance->capability_updates, 1);
}

TEST_F(DeviceTriggerTest, ClaimedDeviceKeepsMatchingWhileConnected)
{
    auto guitar = plug_in(11);
    guitar->wii_type = WiiGuitarHeroGuitar;
    WiiExtTypeActivationTrigger trigger(WiiGuitarHeroGuitar, profile, 1, 0, 0);
    ASSERT_TRUE(trigger.validate(true, false, false));
    EXPECT_TRUE(trigger.validate(false, false, false));
    guitar->still_connected = false;
    EXPECT_FALSE(trigger.validate(false, false, false));
}

TEST_F(DeviceTriggerTest, AClaimedDeviceIsNotAvailableToAnotherProfile)
{
    auto other = make_profile();
    other->profile_id = 2;
    auto guitar = plug_in(11);
    guitar->wii_type = WiiGuitarHeroGuitar;
    WiiExtTypeActivationTrigger first(WiiGuitarHeroGuitar, profile, 1, 0, 0);
    WiiExtTypeActivationTrigger second(WiiGuitarHeroGuitar, other, 2, 0, 0);
    ASSERT_TRUE(first.validate(true, false, false));
    EXPECT_FALSE(second.validate(false, false, false));
    EXPECT_FALSE(second.validate(true, false, false));
    // A second guitar can go to the other profile
    auto guitar2 = plug_in(12);
    guitar2->wii_type = WiiGuitarHeroGuitar;
    EXPECT_TRUE(second.validate(true, false, false));
    EXPECT_EQ(other->get_claimed_device(DeviceSlotKind::WiiExtension, 0), guitar2);
}

TEST_F(DeviceTriggerTest, Ps2ControllerType)
{
    PS2ControllerTypeActivationTrigger trigger(PS2ControllerTypeGuitar, profile, 1, 0, 0);
    auto pad = plug_in(10);
    pad->ps2_type = PS2ControllerTypeDualshock2;
    EXPECT_FALSE(trigger.validate(false, false, false));
    pad->ps2_type = PS2ControllerTypeGuitar;
    EXPECT_TRUE(trigger.validate(true, false, false));
    EXPECT_EQ(profile->get_claimed_device(DeviceSlotKind::PS2, 0), pad);
}

TEST_F(DeviceTriggerTest, UsbTypeHoldsTheDeviceOnlyUntilReset)
{
    // While checking, a USB match is held as a temporary claim (so inputs can preview it), which
    // the list's reset() drops
    UsbTypeActivationTrigger trigger(GuitarHeroGuitar, profile, 1, 1, 0);
    auto guitar = plug_in(10);
    guitar->usb_type = GuitarHeroGuitar;
    EXPECT_TRUE(trigger.validate(false, false, false));
    EXPECT_EQ(profile->get_claimed_device(DeviceSlotKind::USB, 1, true), guitar);
    EXPECT_EQ(profile->get_claimed_device(DeviceSlotKind::USB, 1), nullptr);
    trigger.reset();
    EXPECT_EQ(profile->get_claimed_device(DeviceSlotKind::USB, 1, true), nullptr);
    EXPECT_TRUE(trigger.validate(true, false, false));
    EXPECT_EQ(profile->get_claimed_device(DeviceSlotKind::USB, 1), guitar);
}

TEST_F(DeviceTriggerTest, SpecificUsbDeviceMatchesVidAndPid)
{
    proto_SpecificUsbDevice wanted = {0x1430, 0x4748};
    SpecificUsbDeviceActivationTrigger trigger(wanted, profile, 1, 0, 0);
    auto device = plug_in(10);
    device->vid = 0x1430;
    device->pid = 0x0001;
    EXPECT_FALSE(trigger.validate(false, false, false));
    device->pid = 0x4748;
    EXPECT_TRUE(trigger.validate(true, false, false));
    EXPECT_EQ(profile->get_claimed_device(DeviceSlotKind::USB, 0), device);
}

TEST_F(DeviceTriggerTest, BluetoothTypeAndDevice)
{
    BluetoothTypeActivationTrigger by_type(RockBandGuitar, profile, 1, 0, 0);
    auto device = plug_in(10);
    device->bt_type = RockBandGuitar;
    EXPECT_TRUE(by_type.validate(false, false, false));
    EXPECT_EQ(profile->get_claimed_device(DeviceSlotKind::Bluetooth, 0, true), device);
    by_type.reset();

    proto_SpecificBluetoothDevice wanted = {0x054c, 0x0268};
    SpecificBluetoothDeviceActivationTrigger by_device(wanted, profile, 2, 3, 0);
    EXPECT_FALSE(by_device.validate(false, false, false));
    device->vid = 0x054c;
    device->pid = 0x0268;
    EXPECT_TRUE(by_device.validate(true, false, false));
    EXPECT_EQ(profile->get_claimed_device(DeviceSlotKind::Bluetooth, 3), device);
}

TEST_F(DeviceTriggerTest, MidiChannelIsOneBased)
{
    // The configurator's MIDI channel is 1 - 16; channel 1 is 0 on the wire
    auto keyboard = plug_in(10);
    keyboard->midi_channels = 1u << 0;
    MidiChannelActivationTrigger channel1(1, profile, 1, 0, 0);
    MidiChannelActivationTrigger channel2(2, profile, 2, 0, 0);
    EXPECT_TRUE(channel1.validate(false, false, false));
    EXPECT_FALSE(channel2.validate(false, false, false));
    keyboard->midi_channels = 1u << 15;
    MidiChannelActivationTrigger channel16(16, profile, 3, 0, 0);
    EXPECT_TRUE(channel16.validate(true, false, false));
    EXPECT_EQ(profile->get_claimed_device(DeviceSlotKind::MIDI, 0), keyboard);
}

TEST_F(DeviceTriggerTest, ReportsChangesToTheTool)
{
    WiiExtTypeActivationTrigger trigger(WiiGuitarHeroGuitar, profile, 5, 0, 6);
    trigger.validate(false, false, true);
    EXPECT_TRUE(HIDConfigDevice::sent_events.empty());
    auto guitar = plug_in(11);
    guitar->wii_type = WiiGuitarHeroGuitar;
    trigger.validate(false, false, true);
    trigger.validate(false, false, true);
    ASSERT_EQ(HIDConfigDevice::sent_events.size(), 1u);
    EXPECT_EQ(HIDConfigDevice::sent_events[0].which_event, proto_Event_trigger_tag);
    EXPECT_EQ(HIDConfigDevice::sent_events[0].event.trigger.id, 5u);
    EXPECT_EQ(HIDConfigDevice::sent_events[0].event.trigger.listId, 6u);
    EXPECT_TRUE(HIDConfigDevice::sent_events[0].event.trigger.state);
}

TEST_F(DeviceTriggerTest, UsbModeWithoutAConsoleAlwaysMatches)
{
    UsbModeActivationTrigger trigger(usb_assignment(), profile, 1, 0);
    for (int mode = ModeHid; mode <= ModePdLoader; mode++)
    {
        profile->mode = ConsoleMode(mode);
        EXPECT_TRUE(trigger.validate(false, false, false)) << mode;
    }
    EXPECT_EQ(trigger.assignedDevices(), AssignUsb);
}

TEST_F(DeviceTriggerTest, UsbModeForASpecificConsole)
{
    struct Case
    {
        ConsoleType console;
        std::vector<ConsoleMode> modes;
    };
    const Case cases[] = {
        {ConsolePC, {ModeHid, ModeGuitarHeroArcade, ModeSpice2x, ModePdLoader}},
        {ConsoleOgXbox, {ModeOgXbox}},
        {ConsoleXbox360, {ModeXbox360}},
        {ConsoleXboxOne, {ModeXboxOne}},
        {ConsolePS3, {ModePs3}},
        {ConsolePS4_PS5, {ModePs4, ModePs5}},
        {ConsoleWii_WiiU, {ModeWiiRb}},
        {ConsoleSwitch_Switch2, {ModeSwitch}},
    };
    for (const auto &c : cases)
    {
        auto config = usb_assignment();
        config.has_consoleType = true;
        config.consoleType = c.console;
        UsbModeActivationTrigger trigger(config, profile, 1, 0);
        for (int mode = ModeHid; mode <= ModePdLoader; mode++)
        {
            profile->mode = ConsoleMode(mode);
            bool expected = std::find(c.modes.begin(), c.modes.end(), ConsoleMode(mode)) != c.modes.end();
            EXPECT_EQ(trigger.validate(false, false, false), expected) << c.console << " in mode " << mode;
        }
    }
}

TEST_F(DeviceTriggerTest, UsbModeForcedModeAndOptions)
{
    auto config = usb_assignment();
    UsbModeActivationTrigger plain(config, profile, 1, 0);
    ConsoleMode mode = ModeHid;
    bool enabled = false;
    EXPECT_FALSE(plain.forcedConsoleMode(mode));
    EXPECT_FALSE(plain.xinputOnWindows(enabled));
    EXPECT_FALSE(plain.ps4OrPs5Mode(enabled));

    config.has_forcedType = true;
    config.forcedType = ModePs3;
    config.has_xinputOnWindows = true;
    config.xinputOnWindows = false;
    config.has_ps4OrPs5Mode = true;
    config.ps4OrPs5Mode = true;
    UsbModeActivationTrigger forced(config, profile, 1, 0);
    EXPECT_TRUE(forced.forcedConsoleMode(mode));
    EXPECT_EQ(mode, ModePs3);
    enabled = true;
    EXPECT_TRUE(forced.xinputOnWindows(enabled));
    EXPECT_FALSE(enabled);
    EXPECT_TRUE(forced.ps4OrPs5Mode(enabled));
    EXPECT_TRUE(enabled);

    // A mode number the firmware doesn't know is ignored rather than forced
    config.forcedType = ConsoleMode(13);
    UsbModeActivationTrigger bad(config, profile, 1, 0);
    mode = ModeHid;
    EXPECT_FALSE(bad.forcedConsoleMode(mode));
    EXPECT_EQ(mode, ModeHid);
}

TEST_F(DeviceTriggerTest, BluetoothModeAlwaysMatches)
{
    BluetoothModeActivationTrigger standard(BTStandard, profile, 1, 0);
    BluetoothModeActivationTrigger wiimote(BTWiimote, profile, 2, 0);
    EXPECT_TRUE(standard.validate(false, false, false));
    EXPECT_TRUE(wiimote.validate(true, false, false));
    EXPECT_EQ(standard.assignedDevices(), AssignBluetoothGamepad);
    EXPECT_EQ(wiimote.assignedDevices(), AssignBluetoothWiimote);
    standard.validate(false, false, true);
    standard.validate(false, false, true);
    EXPECT_EQ(HIDConfigDevice::sent_events.size(), 1u);
}

TEST_F(DeviceTriggerTest, WiiExtensionEmulationNeedsAWiiRemoteTalking)
{
    proto_WiimoteAssignment config = proto_WiimoteAssignment_init_default;
    WiiExtensionEmulationActivationTrigger trigger(config, profile, 1, 0);
    EXPECT_EQ(trigger.assignedDevices(), AssignWiimoteExtension);
    EXPECT_FALSE(trigger.validate(false, false, false));
    fake_devices::wii_communicating = true;
    EXPECT_TRUE(trigger.validate(false, false, false));
    EXPECT_TRUE(trigger.validate(true, false, false));
    // There's one extension port: once a profile has it, no other profile can claim it
    ConfigManager::instance().mark_seen_assignment(AssignWiimoteExtension);
    EXPECT_FALSE(trigger.validate(true, false, false));
    EXPECT_TRUE(trigger.validate(false, false, false));
}

TEST_F(DeviceTriggerTest, Ps2EmulationNeedsAConsoleTalking)
{
    proto_PSXAssignment config = proto_PSXAssignment_init_default;
    PS2ControllerEmulationActivationTrigger trigger(config, profile, 1, 0);
    EXPECT_EQ(trigger.assignedDevices(), AssignPsx);
    EXPECT_FALSE(trigger.validate(false, false, false));
    fake_devices::psx_communicating = true;
    EXPECT_TRUE(trigger.validate(true, false, false));
    ConfigManager::instance().mark_seen_assignment(AssignPsx);
    EXPECT_FALSE(trigger.validate(true, false, false));
    // Seen on another port type doesn't matter
    ConfigManager::instance().clear_seen_masks();
    ConfigManager::instance().mark_seen_assignment(AssignWiimoteExtension);
    EXPECT_TRUE(trigger.validate(true, false, false));
}

TEST_F(DeviceTriggerTest, JoybusEmulationNeedsAConsoleTalking)
{
    proto_JoybusAssignment config = proto_JoybusAssignment_init_default;
    JoybusEmulationActivationTrigger trigger(config, profile, 1, 0);
    EXPECT_EQ(trigger.assignedDevices(), AssignJoybus);
    EXPECT_FALSE(trigger.validate(false, false, false));
    fake_devices::psx_communicating = true;
    EXPECT_FALSE(trigger.validate(false, false, false));
    fake_devices::joybus_communicating = true;
    EXPECT_TRUE(trigger.validate(true, false, false));
    ConfigManager::instance().mark_seen_assignment(AssignJoybus);
    EXPECT_FALSE(trigger.validate(true, false, false));
    EXPECT_TRUE(trigger.validate(false, false, false));
}

TEST_F(DeviceTriggerTest, SnesEmulationNeedsAConsoleTalking)
{
    proto_SNESAssignment config = proto_SNESAssignment_init_default;
    SNESEmulationActivationTrigger trigger(config, profile, 1, 0);
    EXPECT_EQ(trigger.assignedDevices(), AssignSnes);
    EXPECT_FALSE(trigger.validate(false, false, false));
    fake_devices::joybus_communicating = true;
    EXPECT_FALSE(trigger.validate(false, false, false));
    fake_devices::snes_communicating = true;
    EXPECT_TRUE(trigger.validate(true, false, false));
    ConfigManager::instance().mark_seen_assignment(AssignSnes);
    EXPECT_FALSE(trigger.validate(true, false, false));
    // Seen on another port type doesn't matter
    ConfigManager::instance().clear_seen_masks();
    ConfigManager::instance().mark_seen_assignment(AssignJoybus);
    EXPECT_TRUE(trigger.validate(true, false, false));
}
