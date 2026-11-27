#pragma once
#include "mappings/base_mapping.hpp"
#include "protocols/controller_reports.hpp"

class GamepadAxisMapping : public AxisMapping
{
public:
    ~GamepadAxisMapping() {}
    GamepadAxisMapping(const proto_Mapping &mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile);
    void update_hid(uint8_t *report);
    void update_wii(uint8_t format, uint8_t *buf);
    void update_switch(uint8_t *report);
    void update_ps2(uint8_t *report);
    void update_gamecube(uint8_t *report);
    void update_n64(uint8_t *report);
    void update_ps3(uint8_t *report);
    void update_ps4(uint8_t *report);
    void update_ps5(uint8_t *report);
    void update_xinput(uint8_t *report);
    void update_ogxbox(uint8_t *report);
    void update_xboxone(uint8_t *report);
    void update_motion(MotionState &motion);

protected:
    // For outputs written as a gamepad axis that calibrate differently, e.g. a wheel's pedals rest at their minimum
    GamepadAxisMapping(const proto_Mapping &mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile, bool trigger);

public:
    enum class StickOutput : uint8_t
    {
        XInput, // also HID and the OG Xbox, which share the 360 instrument layouts
        XboxOne,
        PS2,
        PS3,
        PS4,
        PS5,
    };

protected:
    // Instrument reports keep their own data (velocities, whammy, frets...) where a gamepad's sticks
    // go, so a stick on that side is pressed on the dpad instead. Returns false if this axis isn't a
    // stick on such a side, and should be written as usual.
    bool stick_to_dpad(StickOutput output, uint8_t *buf) const;
};

class PS3GamepadAxisMapping : public GamepadAxisMapping
{
public:
    ~PS3GamepadAxisMapping() {}
    PS3GamepadAxisMapping(const proto_Mapping &mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile);
    void update_ps3(uint8_t *report);
};

// Drum reports keep the pad velocities where the sticks go (on HID, 360 and OG Xbox), so
// gamepad sticks go to the dpad there (see stick_to_dpad). Triggers are unused on drums, so they still pass.
class DrumsGamepadAxisMapping : public GamepadAxisMapping
{
public:
    ~DrumsGamepadAxisMapping() {}
    DrumsGamepadAxisMapping(const proto_Mapping &mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : GamepadAxisMapping(mapping, std::move(input), id, profile) {}
};

class RockBandDrumsGamepadAxisMapping : public DrumsGamepadAxisMapping
{
public:
    ~RockBandDrumsGamepadAxisMapping();
    RockBandDrumsGamepadAxisMapping(const proto_Mapping &mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile);
    void update_xboxone(uint8_t *report);
};

// Turntable reports keep the table buttons where the triggers go and the table velocities,
// effects knob and crossfader where the sticks go, so the triggers are dropped and the sticks
// go to the dpad (e.g. Wii DJ stick noise would otherwise show up as right table movement)
class DJHTurntableGamepadAxisMapping : public GamepadAxisMapping
{
public:
    ~DJHTurntableGamepadAxisMapping() {}
    DJHTurntableGamepadAxisMapping(const proto_Mapping &mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : GamepadAxisMapping(mapping, std::move(input), id, profile) {}
    void update_hid(uint8_t *report) { stick_to_dpad(StickOutput::XInput, report); }
    void update_xinput(uint8_t *report) { stick_to_dpad(StickOutput::XInput, report); }
    void update_ogxbox(uint8_t *report) { stick_to_dpad(StickOutput::XInput, report); }
};

class RockBandGuitarGamepadAxisMapping : public GamepadAxisMapping
{
public:
    ~RockBandGuitarGamepadAxisMapping();
    RockBandGuitarGamepadAxisMapping(const proto_Mapping &mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile);
    void update_xboxone(uint8_t *report);
};

class GamepadButtonMapping : public ButtonMapping
{
public:
    ~GamepadButtonMapping() {}
    GamepadButtonMapping(const proto_Mapping &mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile);
    void update_hid(uint8_t *report);
    void update_wii(uint8_t format, uint8_t *buf);
    void update_wiimote_core(wiimote_buttons *buttons);
    void update_switch(uint8_t *report);
    void update_ps2(uint8_t *report);
    void update_gamecube(uint8_t *report);
    void update_n64(uint8_t *report);
    void update_snes(uint8_t *report);
    void update_nes(uint8_t *report);
    void update_ps3(uint8_t *report);
    void update_ps4(uint8_t *report);
    void update_ps5(uint8_t *report);
    void update_xinput(uint8_t *report);
    void update_ogxbox(uint8_t *report);
    void update_xboxone(uint8_t *report);
    // HID hat values for the dpad bits (up 1, down 2, left 4, right 8), 8 being neutral
    static const uint8_t dpad_bindings[16];

protected:
    GamepadButtonType nintendo_button() const;
    GamepadButtonType gamecube_button() const;
};

class PS3GamepadButtonMapping : public GamepadButtonMapping
{
public:
    ~PS3GamepadButtonMapping() {}
    PS3GamepadButtonMapping(const proto_Mapping &mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile);
    void update_ps3(uint8_t *report);
};
