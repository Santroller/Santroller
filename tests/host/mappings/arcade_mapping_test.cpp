#include <gtest/gtest.h>
#include "mappings/mapping_test_support.hpp"
#include "mappings/switch_arcade_mapping.hpp"
#include "protocols/hid.hpp"
#include "protocols/pdloader.hpp"
#include "protocols/pro_keys.hpp"

// Taiko, Project Diva, Konami (pop'n / beatmania / GuitarFreaks), Rock Band Pro Keys and the
// keyboard / mouse outputs
namespace
{
class Arcade : public MappingTest
{
protected:
    std::shared_ptr<Profile> profile = make_profile();

    template <typename T>
    Driven<T> at(const proto_Mapping &config, uint16_t value)
    {
        auto driven = drive<T>(config, profile);
        driven.set_analog(value);
        return driven;
    }

    template <typename T>
    Driven<T> pressed(const proto_Mapping &config, bool value = true)
    {
        auto driven = drive<T>(config, profile);
        driven.set_digital(value);
        return driven;
    }
};

proto_Output gamepad_output(GamepadButtonType button)
{
    return gamepad_button(button).mapping;
}

proto_Output axis_output(GamepadAxisType axis)
{
    return with_gamepad_axis(base_config(), axis).mapping;
}

SwitchArcadeReport empty_arcade_report()
{
    SwitchArcadeReport report;
    memset(&report, 0, sizeof(report));
    return report;
}
} // namespace

// HORI style Switch arcade report shared by the Taiko and Project Diva controllers

TEST(SwitchArcadeMapping, ButtonsSetTheirBit)
{
    auto report = empty_arcade_report();
    switch_arcade_update_button(gamepad_output(Gamepad_A), true, report);
    switch_arcade_update_button(gamepad_output(Gamepad_Capture), true, report);
    EXPECT_EQ(switch_arcade_buttons(report), SwitchArcade_A | SwitchArcade_Capture);
}

TEST(SwitchArcadeMapping, ReleasedButtonsSetNothing)
{
    auto report = empty_arcade_report();
    switch_arcade_update_button(gamepad_output(Gamepad_A), false, report);
    EXPECT_EQ(switch_arcade_buttons(report), 0);
    EXPECT_EQ(report.hat, 0);
}

TEST(SwitchArcadeMapping, DpadGoesInTheHatsDirectionNibble)
{
    // up, down, left, right in the high nibble until the hat is finished
    auto report = empty_arcade_report();
    switch_arcade_update_button(gamepad_output(Gamepad_DpadUp), true, report);
    switch_arcade_update_button(gamepad_output(Gamepad_DpadRight), true, report);
    EXPECT_EQ(report.hat, 0x10 | 0x80);
    EXPECT_EQ(switch_arcade_buttons(report), 0);
}

TEST(SwitchArcadeMapping, NonGamepadOutputsAreIgnored)
{
    auto report = empty_arcade_report();
    proto_Output output = gamepad_output(Gamepad_A);
    output.which_mapping = proto_Output_divaTouch_tag;
    switch_arcade_update_button(output, true, report);
    EXPECT_EQ(switch_arcade_buttons(report), 0);
}

TEST(SwitchArcadeMapping, SticksAreEightBitWithUpAtZero)
{
    auto report = empty_arcade_report();
    switch_arcade_update_axis(axis_output(Gamepad_LeftStickX), UINT16_MAX, false, report);
    switch_arcade_update_axis(axis_output(Gamepad_LeftStickY), UINT16_MAX, false, report);
    switch_arcade_update_axis(axis_output(Gamepad_RightStickX), 0, false, report);
    switch_arcade_update_axis(axis_output(Gamepad_RightStickY), 0, false, report);
    EXPECT_EQ(report.lx, 0xFF);
    EXPECT_EQ(report.ly, 0x00);
    EXPECT_EQ(report.rx, 0x00);
    EXPECT_EQ(report.ry, 0xFF);
}

TEST(SwitchArcadeMapping, CentredAxesAreLeftAlone)
{
    auto report = empty_arcade_report();
    report.lx = 0x80;
    switch_arcade_update_axis(axis_output(Gamepad_LeftStickX), 0, true, report);
    EXPECT_EQ(report.lx, 0x80);
}

TEST(SwitchArcadeMapping, TriggersAreZLAndZRNearTheEnd)
{
    auto report = empty_arcade_report();
    switch_arcade_update_axis(axis_output(Gamepad_LeftTrigger), 60000, false, report);
    EXPECT_EQ(switch_arcade_buttons(report), 0);
    switch_arcade_update_axis(axis_output(Gamepad_LeftTrigger), 60001, false, report);
    switch_arcade_update_axis(axis_output(Gamepad_RightTrigger), UINT16_MAX, false, report);
    EXPECT_EQ(switch_arcade_buttons(report), SwitchArcade_ZL | SwitchArcade_ZR);
}

// Taiko

TEST_F(Arcade, TaikoTriggersAreDigitalOnPS3)
{
    Report<PS3Gamepad_Data_t> report;
    at<TaikoAxisMapping>(with_gamepad_axis(trigger_config(), Gamepad_LeftTrigger), 60000)->update_ps3(report.buf());
    EXPECT_FALSE(report->l2);
    at<TaikoAxisMapping>(with_gamepad_axis(trigger_config(), Gamepad_RightTrigger), 60001)->update_ps3(report.buf());
    EXPECT_TRUE(report->r2);
}

TEST_F(Arcade, TaikoTriggersAreDigitalOnPS2)
{
    Report<PS2Gamepad_Data_t> report;
    at<TaikoAxisMapping>(with_gamepad_axis(trigger_config(), Gamepad_LeftTrigger), 30000)->update_ps2(report.buf());
    EXPECT_FALSE(report->l2);
    at<TaikoAxisMapping>(with_gamepad_axis(trigger_config(), Gamepad_LeftTrigger), UINT16_MAX)->update_ps2(report.buf());
    EXPECT_TRUE(report->l2);
}

TEST_F(Arcade, TaikoWiiTriggersMatchTheirButtons)
{
    // the left trigger is the same drum zone as dpad left, the right trigger the same as B
    Report<WiiTaikoData_t> by_trigger;
    at<TaikoAxisMapping>(with_gamepad_axis(trigger_config(), Gamepad_LeftTrigger), UINT16_MAX)->update_wii(3, by_trigger.buf());
    at<TaikoAxisMapping>(with_gamepad_axis(trigger_config(), Gamepad_RightTrigger), UINT16_MAX)->update_wii(3, by_trigger.buf());
    Report<WiiTaikoData_t> by_button;
    pressed<TaikoButtonMapping>(gamepad_button(Gamepad_DpadLeft))->update_wii(3, by_button.buf());
    pressed<TaikoButtonMapping>(gamepad_button(Gamepad_B))->update_wii(3, by_button.buf());
    EXPECT_NE(by_trigger->buttons, 0);
    EXPECT_EQ(by_trigger->buttons, by_button->buttons);
}

TEST_F(Arcade, TaikoWiiTriggerBelowThresholdIsOff)
{
    Report<WiiTaikoData_t> report;
    at<TaikoAxisMapping>(with_gamepad_axis(trigger_config(), Gamepad_LeftTrigger), 60000)->update_wii(3, report.buf());
    EXPECT_EQ(report->buttons, 0);
}

TEST_F(Arcade, TaikoSwitchUsesTheArcadeReport)
{
    Report<SwitchArcadeReport> report;
    pressed<TaikoButtonMapping>(gamepad_button(Gamepad_LeftThumbClick))->update_switch(report.buf());
    at<TaikoAxisMapping>(with_gamepad_axis(trigger_config(), Gamepad_RightTrigger), UINT16_MAX)->update_switch(report.buf());
    EXPECT_EQ(switch_arcade_buttons(report.data), SwitchArcade_LS | SwitchArcade_ZR);
}

// Project Diva

namespace
{
proto_Mapping diva_touch(uint32_t electrode)
{
    proto_Mapping config = base_config();
    config.mapping.which_mapping = proto_Output_divaTouch_tag;
    config.mapping.mapping.divaTouch = electrode;
    return config;
}

proto_Mapping diva_slider()
{
    proto_Mapping config = stick_config();
    config.mapping.which_mapping = proto_Output_divaAxis_tag;
    config.mapping.mapping.divaAxis = ProjectDiva_Slider;
    return config;
}
} // namespace

TEST_F(Arcade, DivaTouchElectrodesAreOneBased)
{
    PDLoaderInputReport report;
    pressed<ProjectDivaButtonMapping>(diva_touch(1))->update_pdloader(reinterpret_cast<uint8_t *>(&report));
    pressed<ProjectDivaButtonMapping>(diva_touch(32))->update_pdloader(reinterpret_cast<uint8_t *>(&report));
    EXPECT_EQ(report.slider_touches(), 0x80000001u);
}

TEST_F(Arcade, DivaTouchOutOfRangeIsIgnored)
{
    PDLoaderInputReport report;
    pressed<ProjectDivaButtonMapping>(diva_touch(0))->update_pdloader(reinterpret_cast<uint8_t *>(&report));
    pressed<ProjectDivaButtonMapping>(diva_touch(33))->update_pdloader(reinterpret_cast<uint8_t *>(&report));
    pressed<ProjectDivaButtonMapping>(diva_touch(5), false)->update_pdloader(reinterpret_cast<uint8_t *>(&report));
    EXPECT_EQ(report.slider_touches(), 0u);
}

TEST_F(Arcade, DivaSliderTouchesOneOfThirtyTwoZones)
{
    PDLoaderInputReport left;
    at<ProjectDivaAxisMapping>(diva_slider(), 0)->update_pdloader(reinterpret_cast<uint8_t *>(&left));
    EXPECT_EQ(left.slider_touches(), 1u);
    PDLoaderInputReport right;
    at<ProjectDivaAxisMapping>(diva_slider(), UINT16_MAX)->update_pdloader(reinterpret_cast<uint8_t *>(&right));
    EXPECT_EQ(right.slider_touches(), 1u << 31);
    PDLoaderInputReport middle;
    at<ProjectDivaAxisMapping>(diva_slider(), 0x8000)->update_pdloader(reinterpret_cast<uint8_t *>(&middle));
    EXPECT_EQ(middle.slider_touches(), 1u << 16);
}

TEST_F(Arcade, DivaSliderAtRestTouchesNothing)
{
    PDLoaderInputReport report;
    at<ProjectDivaAxisMapping>(diva_slider(), UINT16_MAX / 2)->update_pdloader(reinterpret_cast<uint8_t *>(&report));
    EXPECT_EQ(report.slider_touches(), 0u);
}

TEST_F(Arcade, DivaButtonsOnSwitch)
{
    Report<SwitchArcadeReport> report;
    pressed<ProjectDivaButtonMapping>(gamepad_button(Gamepad_X))->update_switch(report.buf());
    EXPECT_EQ(switch_arcade_buttons(report.data), SwitchArcade_X);
}

// Konami PS2 controllers, which are digital pads with fixed button layouts

namespace
{
proto_Mapping beatmania(BeatManiaButtonType button)
{
    proto_Mapping config = base_config();
    config.mapping.which_mapping = proto_Output_bmButton_tag;
    config.mapping.mapping.bmButton = button;
    return config;
}

proto_Mapping guitarfreaks(GuitarFreaksButtonType button)
{
    proto_Mapping config = base_config();
    config.mapping.which_mapping = proto_Output_gfButton_tag;
    config.mapping.mapping.gfButton = button;
    return config;
}

proto_Mapping popn(PopNMusicButtonType button)
{
    proto_Mapping config = base_config();
    config.mapping.which_mapping = proto_Output_popnButton_tag;
    config.mapping.mapping.popnButton = button;
    return config;
}

void expect_button(const proto_Mapping &config, GamepadButtonType button)
{
    EXPECT_EQ(config.mapping.which_mapping, proto_Output_gamepadButton_tag);
    EXPECT_EQ(config.mapping.mapping.gamepadButton, button);
}

// A button on L2 / R2 becomes that trigger, fully pressed by the button
void expect_trigger(const proto_Mapping &config, GamepadAxisType axis)
{
    EXPECT_EQ(config.mapping.which_mapping, proto_Output_gamepadAxis_tag);
    EXPECT_EQ(config.mapping.mapping.gamepadAxis, axis);
    EXPECT_TRUE(config.has_pressed);
    EXPECT_EQ(config.pressed, UINT16_MAX);
    EXPECT_TRUE(config.has_released);
    EXPECT_EQ(config.released, 0);
    EXPECT_EQ(config.center, 0);
}
} // namespace

TEST(KonamiMapping, BeatmaniaLayout)
{
    // https://github.com/PCSX2/pcsx2/issues/10176
    expect_button(beatmania_as_gamepad(beatmania(BeatMania_Button1)), Gamepad_X);
    expect_button(beatmania_as_gamepad(beatmania(BeatMania_Button2)), Gamepad_LeftShoulder);
    expect_button(beatmania_as_gamepad(beatmania(BeatMania_Button3)), Gamepad_A);
    expect_button(beatmania_as_gamepad(beatmania(BeatMania_Button4)), Gamepad_RightShoulder);
    expect_button(beatmania_as_gamepad(beatmania(BeatMania_Button5)), Gamepad_B);
    expect_trigger(beatmania_as_gamepad(beatmania(BeatMania_Button6)), Gamepad_LeftTrigger);
    expect_button(beatmania_as_gamepad(beatmania(BeatMania_Button7)), Gamepad_DpadLeft);
    expect_button(beatmania_as_gamepad(beatmania(BeatMania_ScratchClockwise)), Gamepad_DpadUp);
    expect_button(beatmania_as_gamepad(beatmania(BeatMania_ScratchCounterClockwise)), Gamepad_DpadDown);
    expect_trigger(beatmania_as_gamepad(beatmania(BeatMania_Pedal)), Gamepad_RightTrigger);
}

TEST(KonamiMapping, GuitarFreaksLayout)
{
    expect_trigger(guitarfreaks_as_gamepad(guitarfreaks(GuitarFreaks_Red)), Gamepad_RightTrigger);
    expect_button(guitarfreaks_as_gamepad(guitarfreaks(GuitarFreaks_Green)), Gamepad_B);
    expect_button(guitarfreaks_as_gamepad(guitarfreaks(GuitarFreaks_Blue)), Gamepad_Y);
    expect_trigger(guitarfreaks_as_gamepad(guitarfreaks(GuitarFreaks_Tilt)), Gamepad_LeftTrigger);
    expect_button(guitarfreaks_as_gamepad(guitarfreaks(GuitarFreaks_Strum)), Gamepad_DpadUp);
}

TEST(KonamiMapping, PopnTriggerButtons)
{
    expect_trigger(popn_as_gamepad(popn(PopNMusic_Button7)), Gamepad_RightTrigger);
    expect_trigger(popn_as_gamepad(popn(PopNMusic_Button9)), Gamepad_LeftTrigger);
    expect_button(popn_as_gamepad(popn(PopNMusic_Button8)), Gamepad_DpadUp);
}

TEST(KonamiMapping, ConversionKeepsTheInputSettings)
{
    auto config = beatmania(BeatMania_Button1);
    config.inverted = true;
    config.has_debounce = true;
    config.debounce = 7;
    auto converted = beatmania_as_gamepad(config);
    EXPECT_TRUE(converted.inverted);
    EXPECT_TRUE(converted.has_debounce);
    EXPECT_EQ(converted.debounce, 7u);
}

TEST_F(Arcade, KonamiTriggerButtonDrivesTheFullTrigger)
{
    Report<PS2Gamepad_Data_t> report;
    auto pedal = pressed<GamepadAxisMapping>(beatmania_as_gamepad(beatmania(BeatMania_Pedal)));
    pedal->update_ps2(report.buf());
    EXPECT_TRUE(report->r2);
    EXPECT_EQ(report->rightTrigger, 0xFF);
    Report<PS2Gamepad_Data_t> released;
    pedal.set_digital(false);
    pedal->update_ps2(released.buf());
    EXPECT_FALSE(released->r2);
}

// Rock Band Pro Keys key outputs

namespace
{
proto_Mapping pro_key(int32_t key)
{
    proto_Mapping config = base_config();
    config.mapping.which_mapping = proto_Output_proKeySingle_tag;
    config.mapping.mapping.proKeySingle = key;
    return config;
}

proto_Mapping pro_key_range(int32_t count)
{
    proto_Mapping config = base_config();
    config.mapping.which_mapping = proto_Output_proKeyMultiple_tag;
    config.mapping.mapping.proKeyMultiple = count;
    return config;
}

// An input that reports several keys at once, like a MIDI keyboard
class KeyRangeInput : public FakeInput
{
public:
    uint32_t keys = 0;
    uint8_t velocity = 0;
    bool tick_pro_key_range(uint32_t &active_keys, uint8_t *velocities, uint8_t key_count) override
    {
        active_keys = keys & ((key_count >= 32) ? 0xFFFFFFFF : ((1u << key_count) - 1));
        for (uint8_t i = 0; i < key_count; i++)
        {
            velocities[i] = (active_keys & (1u << i)) ? velocity : 0;
        }
        return true;
    }
};

bool key_down(const XInputRockBandKeyboard_Data_t &report, int32_t key)
{
    return pro_keyboard_key_pressed(report.key1, report.key2, report.key3, report.velocities, key);
}

uint16_t key_pressure(const XInputRockBandKeyboard_Data_t &report, int32_t key)
{
    return pro_keyboard_key_pressure(report.key1, report.key2, report.key3, report.velocities, key);
}
} // namespace

TEST_F(Arcade, ProKeySingleKeyWithVelocity)
{
    profile->subtype = ProKeys;
    Report<XInputRockBandKeyboard_Data_t> report;
    auto key = drive<ProKeysKeyMapping>(pro_key(5), profile);
    key.input->analog = 0x8000;
    key.set_digital(true);
    key->update_xinput(report.buf());
    EXPECT_TRUE(key_down(report.data, 5));
    EXPECT_FALSE(key_down(report.data, 4));
    // 7 bit velocity
    EXPECT_EQ(key_pressure(report.data, 5), 0x40 << 9);
}

TEST_F(Arcade, ProKeyReleasedSetsNothing)
{
    Report<XInputRockBandKeyboard_Data_t> report;
    pressed<ProKeysKeyMapping>(pro_key(5), false)->update_xinput(report.buf());
    EXPECT_FALSE(key_down(report.data, 5));
}

TEST_F(Arcade, ProKeyInvertedPressIsFullVelocity)
{
    auto config = pro_key(25);
    config.inverted = true;
    Report<XInputRockBandKeyboard_Data_t> report;
    pressed<ProKeysKeyMapping>(config, false)->update_xinput(report.buf());
    EXPECT_TRUE(key_down(report.data, 25));
    EXPECT_EQ(key_pressure(report.data, 25), 127 << 9);
}

TEST_F(Arcade, ProKeyDebounce)
{
    auto config = pro_key(1);
    config.has_debounce = true;
    config.debounce = 2;
    auto key = pressed<ProKeysKeyMapping>(config);
    fake_time::advance_us(2000);
    key.set_digital(false);
    EXPECT_TRUE(key->wake_pressed());
    fake_time::advance_us(1);
    key.set_digital(false);
    EXPECT_FALSE(key->wake_pressed());
}

TEST_F(Arcade, ProKeyRangeFromAKeyRangeInput)
{
    auto input = std::make_unique<KeyRangeInput>();
    input->keys = (1u << 0) | (1u << 12) | (1u << 24);
    input->velocity = 100;
    ProKeysKeyMapping keys(pro_key_range(25), std::move(input), 1, profile);
    keys.update(false, false);
    Report<XInputRockBandKeyboard_Data_t> report;
    keys.update_xinput(report.buf());
    EXPECT_TRUE(key_down(report.data, 1));
    EXPECT_TRUE(key_down(report.data, 13));
    EXPECT_TRUE(key_down(report.data, 25));
    EXPECT_FALSE(key_down(report.data, 2));
    EXPECT_EQ(key_pressure(report.data, 13), 100 << 9);
}

TEST_F(Arcade, ProKeyRangeOnlyCoversItsKeys)
{
    auto input = std::make_unique<KeyRangeInput>();
    input->keys = 0xFFFFFFFF;
    input->velocity = 100;
    ProKeysKeyMapping keys(pro_key_range(3), std::move(input), 1, profile);
    keys.update(false, false);
    Report<XInputRockBandKeyboard_Data_t> report;
    keys.update_xinput(report.buf());
    EXPECT_TRUE(key_down(report.data, 3));
    EXPECT_FALSE(key_down(report.data, 4));
}

TEST_F(Arcade, ProKeyRangeCountIsCappedAtTheKeyboard)
{
    auto input = std::make_unique<KeyRangeInput>();
    input->keys = 0xFFFFFFFF;
    input->velocity = 1;
    ProKeysKeyMapping keys(pro_key_range(40), std::move(input), 1, profile);
    keys.update(false, false);
    Report<XInputRockBandKeyboard_Data_t> report;
    keys.update_xinput(report.buf());
    EXPECT_TRUE(key_down(report.data, 25));
}

TEST_F(Arcade, ProKeyRangeFromAPlainButtonIsTheFirstKey)
{
    Report<XInputRockBandKeyboard_Data_t> report;
    auto keys = drive<ProKeysKeyMapping>(pro_key_range(10), profile);
    keys.input->analog = UINT16_MAX;
    keys.set_digital(true);
    keys->update_xinput(report.buf());
    EXPECT_TRUE(key_down(report.data, 1));
    EXPECT_FALSE(key_down(report.data, 2));
    EXPECT_EQ(key_pressure(report.data, 1), 127 << 9);
}

// Keyboard and mouse

TEST_F(Arcade, KeyboardKeyGoesToTheProfilesKeyboardState)
{
    proto_Mapping config = base_config();
    config.mapping.which_mapping = proto_Output_keycode_tag;
    config.mapping.mapping.keycode = 0x04; // HID usage for A
    pressed<KeyboardButtonMapping>(config)->update_hid(nullptr);
    EXPECT_TRUE(profile->keyboard_state.is_key_pressed(0x04));
    EXPECT_FALSE(profile->keyboard_state.is_key_pressed(0x05));
}

TEST_F(Arcade, ReleasedKeyboardKeySetsNothing)
{
    proto_Mapping config = base_config();
    config.mapping.which_mapping = proto_Output_keycode_tag;
    config.mapping.mapping.keycode = 0xE0;
    pressed<KeyboardButtonMapping>(config, false)->update_hid(nullptr);
    EXPECT_FALSE(profile->keyboard_state.is_key_pressed(0xE0));
}

TEST_F(Arcade, ConsumerKeyGoesToTheConsumerState)
{
    proto_Mapping config = base_config();
    config.mapping.which_mapping = proto_Output_consumerKey_tag;
    config.mapping.mapping.consumerKey = 0xE9; // volume up
    pressed<KeyboardButtonMapping>(config)->update_hid(nullptr);
    pressed<KeyboardButtonMapping>(config)->update_hid(nullptr);
    ASSERT_EQ(profile->consumer_state.count, 1);
    EXPECT_EQ(profile->consumer_state.keys[0], 0xE9);
}

TEST_F(Arcade, MouseAxesAreSignedDeflection)
{
    auto axis = [](MouseAxisType type) {
        proto_Mapping config = stick_config();
        config.mapping.which_mapping = proto_Output_mouseAxis_tag;
        config.mapping.mapping.mouseAxis = type;
        return config;
    };
    at<MouseAxisMapping>(axis(Mouse_MoveX), UINT16_MAX)->update_hid(nullptr);
    at<MouseAxisMapping>(axis(Mouse_MoveY), 0)->update_hid(nullptr);
    at<MouseAxisMapping>(axis(Mouse_ScrollY), 0x9000)->update_hid(nullptr);
    at<MouseAxisMapping>(axis(Mouse_ScrollX), 0x7000)->update_hid(nullptr);
    EXPECT_EQ(profile->mouse_state.axes[MouseState::X], 32767);
    EXPECT_EQ(profile->mouse_state.axes[MouseState::Y], -32768);
    // vertical scrolling is the wheel, horizontal is pan
    EXPECT_EQ(profile->mouse_state.axes[MouseState::Wheel], 0x1000);
    EXPECT_EQ(profile->mouse_state.axes[MouseState::Pan], -0x1000);
}

TEST_F(Arcade, MouseAxisAtRestLeavesTheStateAlone)
{
    proto_Mapping config = stick_config();
    config.mapping.which_mapping = proto_Output_mouseAxis_tag;
    config.mapping.mapping.mouseAxis = Mouse_MoveX;
    profile->mouse_state.axes[MouseState::X] = 5;
    at<MouseAxisMapping>(config, UINT16_MAX / 2)->update_hid(nullptr);
    EXPECT_EQ(profile->mouse_state.axes[MouseState::X], 5);
}

TEST_F(Arcade, MouseButtons)
{
    auto button = [](MouseButtonType type) {
        proto_Mapping config = base_config();
        config.mapping.which_mapping = proto_Output_mouseButton_tag;
        config.mapping.mapping.mouseButton = type;
        return config;
    };
    pressed<MouseButtonMapping>(button(Mouse_Left))->update_hid(nullptr);
    pressed<MouseButtonMapping>(button(Mouse_Right))->update_hid(nullptr);
    pressed<MouseButtonMapping>(button(Mouse_Middle), false)->update_hid(nullptr);
    // HID mouse buttons: 1 is left, 2 right, 3 middle
    EXPECT_EQ(profile->mouse_state.buttons, 0x01 | 0x02);
}

// Guitar Hero Arcade: the tilt is a signed byte from -127 to 127 like the real guitar's accelerometer
// (Ry in its descriptor), centred where the Xbox 360 guitar's signed tilt is (m_calibrated_value - 32768)
TEST_F(Arcade, GhArcadeTiltIsASignedByte)
{
    proto_Mapping config = trigger_config();
    config.mapping.which_mapping = proto_Output_ghaAxis_tag;
    config.mapping.mapping.ghaAxis = GuitarHeroArcade_Tilt;
    const struct
    {
        uint16_t value;
        int8_t tilt;
    } cases[] = {{UINT16_MAX, 127}, {0xC000, 64}, {0x8000, 0}, {0x4000, -64}, {0x0100, -127}};
    for (const auto &c : cases)
    {
        Report<ArcadeGuitarHeroGuitar_Data_t> report;
        at<GuitarHeroArcadeAxisMapping>(config, c.value)->update_hid(report.buf());
        EXPECT_EQ(report.data.tilt, c.tilt) << std::hex << c.value;
    }
}

// The cabinet picks the side (1 left, 2 right, filled in by GHArcadeGamepadDevice); a Side mapping
// held overrides it to the right side and leaves it alone otherwise
TEST_F(Arcade, GhArcadeSideButtonPicksTheRightSide)
{
    proto_Mapping config = base_config();
    config.mapping.which_mapping = proto_Output_ghaButton_tag;
    config.mapping.mapping.ghaButton = GuitarHeroArcade_Side;
    Report<ArcadeGuitarHeroGuitar_Data_t> report;
    report.data.side = 1;
    pressed<GuitarHeroArcadeButtonMapping>(config, false)->update_hid(report.buf());
    EXPECT_EQ(report.data.side, 1);
    pressed<GuitarHeroArcadeButtonMapping>(config, true)->update_hid(report.buf());
    EXPECT_EQ(report.data.side, 2);
}
