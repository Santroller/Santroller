#pragma once

#include "mappings/gamepad_mapping.hpp"

class TaikoButtonMapping : public PS3GamepadButtonMapping
{
public:
    TaikoButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile);
    void update_switch(uint8_t *report) override;
    void update_wii(uint8_t format, uint8_t *report) override;
};

class TaikoAxisMapping : public PS3GamepadAxisMapping
{
public:
    TaikoAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile);
    void update_switch(uint8_t *report) override;
    void update_ps3(uint8_t *report) override;
    void update_wii(uint8_t format, uint8_t *report) override;
    void update_ps2(uint8_t *report) override;
};