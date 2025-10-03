#pragma once
#include <stdint.h>
#include "input/input.hpp"
#include "input.pb.h"
#include "state/base.hpp"
#include "protocols/controller_reports.hpp"
#include "protocols/hid.hpp"
#include "protocols/wii.hpp"
#include "protocols/og_xbox.hpp"
#include "protocols/ps2.hpp"
#include "protocols/ps3.hpp"
#include "protocols/ps4.hpp"
#include "protocols/xbox_one.hpp"
#include "protocols/xinput.hpp"
#include "protocols/switch.hpp"
#include "console_mode.h"
#include <memory>
class Mapping
{
public:
    Mapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id) : m_mapping(mapping), m_id(id), m_input(std::move(input)) {}
    virtual ~Mapping() {}
    virtual void update(bool full_poll) = 0;
    virtual void update_hid(uint8_t *report) = 0;
    virtual void update_wii(uint8_t *report) = 0;
    virtual void update_switch(uint8_t *report) = 0;
    virtual void update_ps2(uint8_t *report) = 0;
    virtual void update_ps3(uint8_t *report) = 0;
    virtual void update_ps4(uint8_t *report) = 0;
    virtual void update_xinput(uint8_t *report) = 0;
    virtual void update_ogxbox(uint8_t *report) = 0;
    void update_digital(bool full_poll);
    uint16_t calibrate(float val, float max, float min, float deadzone, float center, bool trigger);

protected:
    proto_Mapping m_mapping;
    uint16_t m_id;
    uint32_t m_lastValueRaw = 0;
    uint32_t m_lastValueTrigger = 0;
    std::unique_ptr<Input> m_input;
};

class ButtonMapping : public Mapping
{
public:
    ~ButtonMapping() {}
    ButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id) : Mapping(mapping, std::move(input), id) {}
    void update(bool full_poll);

protected:
    bool m_lastValue = false;
    bool m_calibratedValue = false;
    uint64_t m_lastPoll = 0;
};
class AxisMapping : public Mapping
{
public:
    ~AxisMapping() {}
    AxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, bool trigger) : Mapping(mapping, std::move(input), id), m_trigger(trigger) {}
    void update(bool full_poll);

protected:
    uint32_t m_lastValue = 0;
    uint32_t m_calibratedValue = 0;
    bool m_trigger;
};
class GamepadAxisMapping : public AxisMapping
{
public:
    ~GamepadAxisMapping() {}
    GamepadAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id);
    void update_hid(uint8_t *report);
    void update_wii(uint8_t *report);
    void update_switch(uint8_t *report);
    void update_ps2(uint8_t *report);
    void update_ps3(uint8_t *report);
    void update_ps4(uint8_t *report);
    void update_xinput(uint8_t *report);
    void update_ogxbox(uint8_t *report);
};
class GuitarHeroGuitarButtonMapping : public ButtonMapping
{
public:
    ~GuitarHeroGuitarButtonMapping() {}
    GuitarHeroGuitarButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id);
    void update_hid(uint8_t *report);
    void update_wii(uint8_t *report);
    void update_switch(uint8_t *report);
    void update_ps2(uint8_t *report);
    void update_ps3(uint8_t *report);
    void update_ps4(uint8_t *report);
    void update_xinput(uint8_t *report);
    void update_ogxbox(uint8_t *report);
};
class GuitarHeroGuitarAxisMapping : public AxisMapping
{
public:
    ~GuitarHeroGuitarAxisMapping() {}
    GuitarHeroGuitarAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id);
    void update_hid(uint8_t *report);
    void update_wii(uint8_t *report);
    void update_switch(uint8_t *report);
    void update_ps2(uint8_t *report);
    void update_ps3(uint8_t *report);
    void update_ps4(uint8_t *report);
    void update_xinput(uint8_t *report);
    void update_ogxbox(uint8_t *report);
};

class GamepadButtonMapping : public ButtonMapping
{
public:
    ~GamepadButtonMapping() {}
    GamepadButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id);
    void update_hid(uint8_t *report);
    void update_wii(uint8_t *report);
    void update_switch(uint8_t *report);
    void update_ps2(uint8_t *report);
    void update_ps3(uint8_t *report);
    void update_ps4(uint8_t *report);
    void update_xinput(uint8_t *report);
    void update_ogxbox(uint8_t *report);
};

class ActivationTrigger
{
public:
    ActivationTrigger(proto_ActivationTrigger activation_trigger, std::unique_ptr<Input> input, uint16_t profile_id);
    ~ActivationTrigger() {}
    void update(bool tool_closed);

protected:
    proto_ActivationTrigger m_activation_trigger;
    std::unique_ptr<Input> m_input;
    uint16_t m_profile_id;
};