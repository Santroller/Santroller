#pragma once
#include "mappings/gamepad_mapping.hpp"

// Wheels send a gamepad report, so each wheel axis is written as its gamepad equivalent: steering on
// the left stick, the accelerator and brake on the triggers. Only PC HID and XInput have room for the
// clutch and handbrake too. PS4 wheels are G29s, which have their own report, and PS5 wheels are
// assumed to send the same data. PS3 wheels are G27s, GameCube wheels Speed Forces and Xbox One wheels
// G920s, with their own reports too.
class WheelAxisMapping : public GamepadAxisMapping
{
public:
    ~WheelAxisMapping() {}
    WheelAxisMapping(const proto_Mapping &mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile);
    // From the gamepad axis the wheel axis is sent on (see wheel_gamepad_axis)
    WheelAxisMapping(const proto_Mapping &gamepad_mapping, WheelAxisType axis, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile);
    void update_hid(uint8_t *report);
    void update_wii(uint8_t format, uint8_t *buf);
    void update_switch(uint8_t *report);
    void update_ps2(uint8_t *report);
    void update_gamecube(uint8_t *report);
    void update_n64(uint8_t *report);
    void update_ps3(uint8_t *report);
    void update_ps4(uint8_t *report);
    void update_ps5(uint8_t *report);
    void update_ogxbox(uint8_t *report);
    void update_xboxone(uint8_t *report);

private:
    WheelAxisType m_axis;
    // The clutch and handbrake have nowhere to go in a console's gamepad report
    bool on_gamepad() const { return m_axis == Wheel_Steering || m_axis == Wheel_Accelerator || m_axis == Wheel_Brake; }
};

// The buttons only a G29 has (its H shifter and the buttons around its dial), so they only go
// anywhere on a PS4 or PS5, apart from the gears which PS3 and Xbox One wheels also have
class WheelButtonMapping : public ButtonMapping
{
public:
    ~WheelButtonMapping() {}
    WheelButtonMapping(const proto_Mapping &mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile);
    void update_ps3(uint8_t *report);
    void update_ps4(uint8_t *report);
    void update_ps5(uint8_t *report);
    void update_xboxone(uint8_t *report);
};

// A gamepad axis with no wheel equivalent (see gamepad_wheel_axis) on a wheel, which the PS3, GameCube and
// Xbox One wheels' reports have no room for
class WheelGamepadAxisMapping : public GamepadAxisMapping
{
public:
    ~WheelGamepadAxisMapping() {}
    WheelGamepadAxisMapping(const proto_Mapping &mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile);
    void update_ps3(uint8_t *report);
    void update_gamecube(uint8_t *report);
    void update_xboxone(uint8_t *report);
};

// A gamepad button on a wheel: a PS3 wheel is a G27 with its own report, a GameCube wheel's paddles
// are its analog triggers, and an original Xbox wheel's are A and X
class WheelGamepadButtonMapping : public GamepadButtonMapping
{
public:
    ~WheelGamepadButtonMapping() {}
    WheelGamepadButtonMapping(const proto_Mapping &mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile);
    void update_ps3(uint8_t *report);
    void update_gamecube(uint8_t *report);
    void update_ogxbox(uint8_t *report);
};
