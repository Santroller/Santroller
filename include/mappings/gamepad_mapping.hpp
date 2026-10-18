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
};

class PS3GamepadAxisMapping : public GamepadAxisMapping
{
public:
    ~PS3GamepadAxisMapping() {}
    PS3GamepadAxisMapping(const proto_Mapping &mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile);
    void update_ps3(uint8_t *report);
};

// Drum reports keep the pad velocities where the sticks go (on HID, 360 and OG Xbox), so
// gamepad sticks can't be written there (e.g. a passed through controller's stick noise
// would overwrite every hit's velocity). Triggers are unused on drums, so they still pass.
class DrumsGamepadAxisMapping : public GamepadAxisMapping
{
public:
    ~DrumsGamepadAxisMapping() {}
    DrumsGamepadAxisMapping(const proto_Mapping &mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : GamepadAxisMapping(mapping, std::move(input), id, profile) {}
    void update_hid(uint8_t *report);
    void update_xinput(uint8_t *report);
    void update_ogxbox(uint8_t *report);

private:
    bool is_stick() const;
};

class RockBandDrumsGamepadAxisMapping : public DrumsGamepadAxisMapping
{
public:
    ~RockBandDrumsGamepadAxisMapping();
    RockBandDrumsGamepadAxisMapping(const proto_Mapping &mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile);
    void update_xboxone(uint8_t *report);
};

// Turntable reports keep the table buttons where the triggers go and the table velocities,
// effects knob and crossfader where the sticks go, so gamepad axes can't be written there
// (e.g. Wii DJ stick noise would show up as right table movement)
class DJHTurntableGamepadAxisMapping : public GamepadAxisMapping
{
public:
    ~DJHTurntableGamepadAxisMapping() {}
    DJHTurntableGamepadAxisMapping(const proto_Mapping &mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : GamepadAxisMapping(mapping, std::move(input), id, profile) {}
    void update_hid(uint8_t *report) { (void)report; }
    void update_xinput(uint8_t *report) { (void)report; }
    void update_ogxbox(uint8_t *report) { (void)report; }
};

class RockBandGuitarGamepadAxisMapping : public GamepadAxisMapping
{
public:
    ~RockBandGuitarGamepadAxisMapping();
    RockBandGuitarGamepadAxisMapping(const proto_Mapping &mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile);
    void update_xboxone(uint8_t *report);
    void update_xinput(uint8_t *report);
    void update_ogxbox(uint8_t *report);
    void update_hid(uint8_t *report);
private:
    bool is_stick() const;
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
