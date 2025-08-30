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
    Mapping(uint16_t id): m_id(id) {}
    virtual ~Mapping() {}
    virtual void update(bool full_poll)=0;
    virtual void update_hid(uint8_t *report)=0;
    virtual void update_wii(uint8_t *report)=0;
    virtual void update_switch(uint8_t* report)=0;
    virtual void update_ps2(uint8_t* report)=0;
    virtual void update_ps3(uint8_t* report)=0;
    virtual void update_ps4(uint8_t* report)=0;
    virtual void update_xinput(uint8_t* report)=0;
    virtual void update_ogxbox(uint8_t* report)=0;
protected:
    uint16_t m_id;
};

class GamepadAxisMapping : public Mapping
{
public:
    ~GamepadAxisMapping() {}
    GamepadAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id);
    void update(bool full_poll);
    void update_hid(uint8_t *report);
    void update_wii(uint8_t *report);
    void update_switch(uint8_t* report);
    void update_ps2(uint8_t* report);
    void update_ps3(uint8_t* report);
    void update_ps4(uint8_t* report);
    void update_xinput(uint8_t* report);
    void update_ogxbox(uint8_t* report);

private:
    proto_Mapping m_mapping;
    std::unique_ptr<Input> m_input;
    uint32_t m_lastValue = 0;
    uint32_t m_calibratedValue = 0;
    bool m_trigger;
};
class GuitarHeroGuitarAxisMapping : public Mapping
{
public:
    ~GuitarHeroGuitarAxisMapping() {}
    GuitarHeroGuitarAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id);
    void update(bool full_poll);
    void update_hid(uint8_t *report);
    void update_wii(uint8_t *report);
    void update_switch(uint8_t* report);
    void update_ps2(uint8_t* report);
    void update_ps3(uint8_t* report);
    void update_ps4(uint8_t* report);
    void update_xinput(uint8_t* report);
    void update_ogxbox(uint8_t* report);

private:
    proto_Mapping m_mapping;
    std::unique_ptr<Input> m_input;
    uint32_t m_lastValue = 0;
    uint32_t m_calibratedValue = 0;
    bool m_trigger;
};

class GamepadButtonMapping : public Mapping
{
public:
    ~GamepadButtonMapping() {}
    GamepadButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id);
    void update(bool full_poll);
    void update_hid(uint8_t *report);
    void update_wii(uint8_t *report);
    void update_switch(uint8_t* report);
    void update_ps2(uint8_t* report);
    void update_ps3(uint8_t* report);
    void update_ps4(uint8_t* report);
    void update_xinput(uint8_t* report);
    void update_ogxbox(uint8_t* report);

private:
    proto_Mapping m_mapping;
    std::unique_ptr<Input> m_input;
    bool m_lastValue = false;
    bool m_calibratedValue = false;
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