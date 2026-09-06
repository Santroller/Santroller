#pragma once
#include "mappings/base_mapping.hpp"
#include "protocols/controller_reports.hpp"

class GamepadAxisMapping : public AxisMapping
{
public:
    ~GamepadAxisMapping() {}
    GamepadAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile);
    void update_hid(uint8_t *report);
    void update_wii(uint8_t format, uint8_t *buf);
    void update_switch(uint8_t *report);
    void update_ps2(uint8_t *report);
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
    PS3GamepadAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile);
    void update_ps3(uint8_t *report);
};

class RockBandDrumsGamepadAxisMapping : public GamepadAxisMapping
{
public:
    ~RockBandDrumsGamepadAxisMapping();
    RockBandDrumsGamepadAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile);
    void update_xboxone(uint8_t *report);
};

class RockBandGuitarGamepadAxisMapping : public GamepadAxisMapping
{
public:
    ~RockBandGuitarGamepadAxisMapping();
    RockBandGuitarGamepadAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile);
    void update_xboxone(uint8_t *report);
};

class GamepadButtonMapping : public ButtonMapping
{
public:
    ~GamepadButtonMapping() {}
    GamepadButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile);
    void update_hid(uint8_t *report);
    void update_wii(uint8_t format, uint8_t *buf);
    void update_wiimote_core(wiimote_buttons *buttons);
    void update_switch(uint8_t *report);
    void update_ps2(uint8_t *report);
    void update_ps3(uint8_t *report);
    void update_ps4(uint8_t *report);
    void update_ps5(uint8_t *report);
    void update_xinput(uint8_t *report);
    void update_ogxbox(uint8_t *report);
    void update_xboxone(uint8_t *report);
    static const uint8_t dpad_bindings[15];
};

class PS3GamepadButtonMapping : public GamepadButtonMapping
{
public:
    ~PS3GamepadButtonMapping() {}
    PS3GamepadButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile);
    void update_ps3(uint8_t *report);
};
