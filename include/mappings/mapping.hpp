#pragma once
#include <stdint.h>
#include "input/input.hpp"
#include "input.pb.h"
#include "protocols/controller_reports.hpp"
#include "protocols/hid.hpp"
#include "protocols/wii.hpp"
#include "protocols/og_xbox.hpp"
#include "protocols/ps2.hpp"
#include "protocols/ps3.hpp"
#include "protocols/ps4.hpp"
#include "protocols/ps5.hpp"
#include "protocols/xbox_one.hpp"
#include "protocols/xinput.hpp"
#include "protocols/switch.hpp"
#include <memory>
class Mapping
{
public:
    Mapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, uint32_t profile) : m_mapping(mapping), m_id(id), m_profile(profile), m_input(std::move(input)) {}
    virtual ~Mapping() {}
    virtual void update(bool full_poll) = 0;
    virtual void update_hid(uint8_t *report) = 0;
    virtual void update_wii(uint8_t *report) = 0;
    virtual void update_switch(uint8_t *report) = 0;
    virtual void update_ps2(uint8_t *report) = 0;
    virtual void update_ps3(uint8_t *report) = 0;
    virtual void update_ps4(uint8_t *report) = 0;
    virtual void update_ps5(uint8_t *report) = 0;
    virtual void update_xinput(uint8_t *report) = 0;
    virtual void update_ogxbox(uint8_t *report) = 0;
    virtual void update_xboxone(uint8_t *report) = 0;
    void update_digital(bool full_poll);
    uint16_t calibrate(float val, float max, float min, float deadzone, float center, bool trigger);

protected:
    proto_Mapping m_mapping;
    uint16_t m_id;
    uint32_t m_profile;
    uint32_t m_lastValueRaw = 0;
    uint32_t m_lastValueTrigger = 0;
    bool m_resend = false;
    std::unique_ptr<Input> m_input;
};

class ButtonMapping : public Mapping
{
public:
    ~ButtonMapping() {}
    ButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, uint32_t profile) : Mapping(mapping, std::move(input), id, profile) {}
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
    AxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, uint32_t profile, bool trigger) : Mapping(mapping, std::move(input), id, profile), m_trigger(trigger) {}
    void update(bool full_poll);

protected:
    uint32_t m_lastValue = 0;
    uint32_t m_calibratedValue = 0;
    bool m_centered = false;
    bool m_trigger;
};
class GamepadAxisMapping : public AxisMapping
{
public:
    ~GamepadAxisMapping() {}
    GamepadAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, uint32_t profile);
    void update_hid(uint8_t *report);
    void update_wii(uint8_t *report);
    void update_switch(uint8_t *report);
    void update_ps2(uint8_t *report);
    void update_ps3(uint8_t *report);
    void update_ps4(uint8_t *report);
    void update_ps5(uint8_t *report);
    void update_xinput(uint8_t *report);
    void update_ogxbox(uint8_t *report);
    void update_xboxone(uint8_t *report);
};
class DJMaxTurntableButtonMapping : public ButtonMapping
{
public:
    ~DJMaxTurntableButtonMapping() {}
    DJMaxTurntableButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, uint32_t profile);
    void update_hid(uint8_t *report);
    void update_wii(uint8_t *report);
    void update_switch(uint8_t *report);
    void update_ps2(uint8_t *report);
    void update_ps3(uint8_t *report);
    void update_ps4(uint8_t *report);
    void update_ps5(uint8_t *report);
    void update_xinput(uint8_t *report);
    void update_ogxbox(uint8_t *report);
    void update_xboxone(uint8_t *report);
};
class DJMaxTurntableAxisMapping : public AxisMapping
{
public:
    ~DJMaxTurntableAxisMapping() {}
    DJMaxTurntableAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, uint32_t profile);
    void update_hid(uint8_t *report);
    void update_wii(uint8_t *report);
    void update_switch(uint8_t *report);
    void update_ps2(uint8_t *report);
    void update_ps3(uint8_t *report);
    void update_ps4(uint8_t *report);
    void update_ps5(uint8_t *report);
    void update_xinput(uint8_t *report);
    void update_ogxbox(uint8_t *report);
    void update_xboxone(uint8_t *report);
};
class GuitarFreaksButtonMapping : public ButtonMapping
{
public:
    ~GuitarFreaksButtonMapping() {}
    GuitarFreaksButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, uint32_t profile);
    void update_hid(uint8_t *report);
    void update_wii(uint8_t *report);
    void update_switch(uint8_t *report);
    void update_ps2(uint8_t *report);
    void update_ps3(uint8_t *report);
    void update_ps4(uint8_t *report);
    void update_ps5(uint8_t *report);
    void update_xinput(uint8_t *report);
    void update_ogxbox(uint8_t *report);
    void update_xboxone(uint8_t *report);
};
class GuitarFreaksAxisMapping : public AxisMapping
{
public:
    ~GuitarFreaksAxisMapping() {}
    GuitarFreaksAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, uint32_t profile);
    void update_hid(uint8_t *report);
    void update_wii(uint8_t *report);
    void update_switch(uint8_t *report);
    void update_ps2(uint8_t *report);
    void update_ps3(uint8_t *report);
    void update_ps4(uint8_t *report);
    void update_ps5(uint8_t *report);
    void update_xinput(uint8_t *report);
    void update_ogxbox(uint8_t *report);
    void update_xboxone(uint8_t *report);
};
class GuitarHeroArcadeButtonMapping : public ButtonMapping
{
public:
    ~GuitarHeroArcadeButtonMapping() {}
    GuitarHeroArcadeButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, uint32_t profile);
    void update_hid(uint8_t *report);
    void update_wii(uint8_t *report);
    void update_switch(uint8_t *report);
    void update_ps2(uint8_t *report);
    void update_ps3(uint8_t *report);
    void update_ps4(uint8_t *report);
    void update_ps5(uint8_t *report);
    void update_xinput(uint8_t *report);
    void update_ogxbox(uint8_t *report);
    void update_xboxone(uint8_t *report);
};
class GuitarHeroArcadeAxisMapping : public AxisMapping
{
public:
    ~GuitarHeroArcadeAxisMapping() {}
    GuitarHeroArcadeAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, uint32_t profile);
    void update_hid(uint8_t *report);
    void update_wii(uint8_t *report);
    void update_switch(uint8_t *report);
    void update_ps2(uint8_t *report);
    void update_ps3(uint8_t *report);
    void update_ps4(uint8_t *report);
    void update_ps5(uint8_t *report);
    void update_xinput(uint8_t *report);
    void update_ogxbox(uint8_t *report);
    void update_xboxone(uint8_t *report);
};
class LiveGuitarButtonMapping : public ButtonMapping
{
public:
    ~LiveGuitarButtonMapping() {}
    LiveGuitarButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, uint32_t profile);
    void update_hid(uint8_t *report);
    void update_wii(uint8_t *report);
    void update_switch(uint8_t *report);
    void update_ps2(uint8_t *report);
    void update_ps3(uint8_t *report);
    void update_ps4(uint8_t *report);
    void update_ps5(uint8_t *report);
    void update_xinput(uint8_t *report);
    void update_ogxbox(uint8_t *report);
    void update_xboxone(uint8_t *report);
};
class LiveGuitarAxisMapping : public AxisMapping
{
public:
    ~LiveGuitarAxisMapping() {}
    LiveGuitarAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, uint32_t profile);
    void update_hid(uint8_t *report);
    void update_wii(uint8_t *report);
    void update_switch(uint8_t *report);
    void update_ps2(uint8_t *report);
    void update_ps3(uint8_t *report);
    void update_ps4(uint8_t *report);
    void update_ps5(uint8_t *report);
    void update_xinput(uint8_t *report);
    void update_ogxbox(uint8_t *report);
    void update_xboxone(uint8_t *report);
};
class PopNMusicButtonMapping : public ButtonMapping
{
public:
    ~PopNMusicButtonMapping() {}
    PopNMusicButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, uint32_t profile);
    void update_hid(uint8_t *report);
    void update_wii(uint8_t *report);
    void update_switch(uint8_t *report);
    void update_ps2(uint8_t *report);
    void update_ps3(uint8_t *report);
    void update_ps4(uint8_t *report);
    void update_ps5(uint8_t *report);
    void update_xinput(uint8_t *report);
    void update_ogxbox(uint8_t *report);
    void update_xboxone(uint8_t *report);
};
class PopNMusicAxisMapping : public AxisMapping
{
public:
    ~PopNMusicAxisMapping() {}
    PopNMusicAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, uint32_t profile);
    void update_hid(uint8_t *report);
    void update_wii(uint8_t *report);
    void update_switch(uint8_t *report);
    void update_ps2(uint8_t *report);
    void update_ps3(uint8_t *report);
    void update_ps4(uint8_t *report);
    void update_ps5(uint8_t *report);
    void update_xinput(uint8_t *report);
    void update_ogxbox(uint8_t *report);
    void update_xboxone(uint8_t *report);
};
class ProGuitarButtonMapping : public ButtonMapping
{
public:
    ~ProGuitarButtonMapping() {}
    ProGuitarButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, uint32_t profile);
    void update_hid(uint8_t *report);
    void update_wii(uint8_t *report);
    void update_switch(uint8_t *report);
    void update_ps2(uint8_t *report);
    void update_ps3(uint8_t *report);
    void update_ps4(uint8_t *report);
    void update_ps5(uint8_t *report);
    void update_xinput(uint8_t *report);
    void update_ogxbox(uint8_t *report);
    void update_xboxone(uint8_t *report);
};
class ProGuitarAxisMapping : public AxisMapping
{
public:
    ~ProGuitarAxisMapping() {}
    ProGuitarAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, uint32_t profile);
    void update_hid(uint8_t *report);
    void update_wii(uint8_t *report);
    void update_switch(uint8_t *report);
    void update_ps2(uint8_t *report);
    void update_ps3(uint8_t *report);
    void update_ps4(uint8_t *report);
    void update_ps5(uint8_t *report);
    void update_xinput(uint8_t *report);
    void update_ogxbox(uint8_t *report);
    void update_xboxone(uint8_t *report);
};
class ProjectDivaButtonMapping : public ButtonMapping
{
public:
    ~ProjectDivaButtonMapping() {}
    ProjectDivaButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, uint32_t profile);
    void update_hid(uint8_t *report);
    void update_wii(uint8_t *report);
    void update_switch(uint8_t *report);
    void update_ps2(uint8_t *report);
    void update_ps3(uint8_t *report);
    void update_ps4(uint8_t *report);
    void update_ps5(uint8_t *report);
    void update_xinput(uint8_t *report);
    void update_ogxbox(uint8_t *report);
    void update_xboxone(uint8_t *report);
};
class ProjectDivaAxisMapping : public AxisMapping
{
public:
    ~ProjectDivaAxisMapping() {}
    ProjectDivaAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, uint32_t profile);
    void update_hid(uint8_t *report);
    void update_wii(uint8_t *report);
    void update_switch(uint8_t *report);
    void update_ps2(uint8_t *report);
    void update_ps3(uint8_t *report);
    void update_ps4(uint8_t *report);
    void update_ps5(uint8_t *report);
    void update_xinput(uint8_t *report);
    void update_ogxbox(uint8_t *report);
    void update_xboxone(uint8_t *report);
};
class ProKeysButtonMapping : public ButtonMapping
{
public:
    ~ProKeysButtonMapping() {}
    ProKeysButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, uint32_t profile);
    void update_hid(uint8_t *report);
    void update_wii(uint8_t *report);
    void update_switch(uint8_t *report);
    void update_ps2(uint8_t *report);
    void update_ps3(uint8_t *report);
    void update_ps4(uint8_t *report);
    void update_ps5(uint8_t *report);
    void update_xinput(uint8_t *report);
    void update_ogxbox(uint8_t *report);
    void update_xboxone(uint8_t *report);
};
class ProKeysAxisMapping : public AxisMapping
{
public:
    ~ProKeysAxisMapping() {}
    ProKeysAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, uint32_t profile);
    void update_hid(uint8_t *report);
    void update_wii(uint8_t *report);
    void update_switch(uint8_t *report);
    void update_ps2(uint8_t *report);
    void update_ps3(uint8_t *report);
    void update_ps4(uint8_t *report);
    void update_ps5(uint8_t *report);
    void update_xinput(uint8_t *report);
    void update_ogxbox(uint8_t *report);
    void update_xboxone(uint8_t *report);
};
class RockBandDrumsButtonMapping : public ButtonMapping
{
public:
    ~RockBandDrumsButtonMapping() {}
    RockBandDrumsButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, uint32_t profile);
    void update_hid(uint8_t *report);
    void update_wii(uint8_t *report);
    void update_switch(uint8_t *report);
    void update_ps2(uint8_t *report);
    void update_ps3(uint8_t *report);
    void update_ps4(uint8_t *report);
    void update_ps5(uint8_t *report);
    void update_xinput(uint8_t *report);
    void update_ogxbox(uint8_t *report);
    void update_xboxone(uint8_t *report);
};
class RockBandDrumsAxisMapping : public AxisMapping
{
public:
    ~RockBandDrumsAxisMapping() {}
    RockBandDrumsAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, uint32_t profile);
    void update_hid(uint8_t *report);
    void update_wii(uint8_t *report);
    void update_switch(uint8_t *report);
    void update_ps2(uint8_t *report);
    void update_ps3(uint8_t *report);
    void update_ps4(uint8_t *report);
    void update_ps5(uint8_t *report);
    void update_xinput(uint8_t *report);
    void update_ogxbox(uint8_t *report);
    void update_xboxone(uint8_t *report);
};
class TaikoButtonMapping : public ButtonMapping
{
public:
    ~TaikoButtonMapping() {}
    TaikoButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, uint32_t profile);
    void update_hid(uint8_t *report);
    void update_wii(uint8_t *report);
    void update_switch(uint8_t *report);
    void update_ps2(uint8_t *report);
    void update_ps3(uint8_t *report);
    void update_ps4(uint8_t *report);
    void update_ps5(uint8_t *report);
    void update_xinput(uint8_t *report);
    void update_ogxbox(uint8_t *report);
    void update_xboxone(uint8_t *report);
};
class TaikoAxisMapping : public AxisMapping
{
public:
    ~TaikoAxisMapping() {}
    TaikoAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, uint32_t profile);
    void update_hid(uint8_t *report);
    void update_wii(uint8_t *report);
    void update_switch(uint8_t *report);
    void update_ps2(uint8_t *report);
    void update_ps3(uint8_t *report);
    void update_ps4(uint8_t *report);
    void update_ps5(uint8_t *report);
    void update_xinput(uint8_t *report);
    void update_ogxbox(uint8_t *report);
    void update_xboxone(uint8_t *report);
};
class WheelButtonMapping : public ButtonMapping
{
public:
    ~WheelButtonMapping() {}
    WheelButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, uint32_t profile);
    void update_hid(uint8_t *report);
    void update_wii(uint8_t *report);
    void update_switch(uint8_t *report);
    void update_ps2(uint8_t *report);
    void update_ps3(uint8_t *report);
    void update_ps4(uint8_t *report);
    void update_ps5(uint8_t *report);
    void update_xinput(uint8_t *report);
    void update_ogxbox(uint8_t *report);
    void update_xboxone(uint8_t *report);
};
class WheelAxisMapping : public AxisMapping
{
public:
    ~WheelAxisMapping() {}
    WheelAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, uint32_t profile);
    void update_hid(uint8_t *report);
    void update_wii(uint8_t *report);
    void update_switch(uint8_t *report);
    void update_ps2(uint8_t *report);
    void update_ps3(uint8_t *report);
    void update_ps4(uint8_t *report);
    void update_ps5(uint8_t *report);
    void update_xinput(uint8_t *report);
    void update_ogxbox(uint8_t *report);
    void update_xboxone(uint8_t *report);
};
class KeyboardButtonMapping : public ButtonMapping
{
public:
    ~KeyboardButtonMapping() {}
    KeyboardButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, uint32_t profile);
    void update_hid(uint8_t *report);
    void update_wii(uint8_t *report);
    void update_switch(uint8_t *report);
    void update_ps2(uint8_t *report);
    void update_ps3(uint8_t *report);
    void update_ps4(uint8_t *report);
    void update_ps5(uint8_t *report);
    void update_xinput(uint8_t *report);
    void update_ogxbox(uint8_t *report);
    void update_xboxone(uint8_t *report);
};
class MouseAxisMapping : public AxisMapping
{
public:
    ~MouseAxisMapping() {}
    MouseAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, uint32_t profile);
    void update_hid(uint8_t *report);
    void update_wii(uint8_t *report);
    void update_switch(uint8_t *report);
    void update_ps2(uint8_t *report);
    void update_ps3(uint8_t *report);
    void update_ps4(uint8_t *report);
    void update_ps5(uint8_t *report);
    void update_xinput(uint8_t *report);
    void update_ogxbox(uint8_t *report);
    void update_xboxone(uint8_t *report);
};
class MouseButtonMapping : public ButtonMapping
{
public:
    ~MouseButtonMapping() {}
    MouseButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, uint32_t profile);
    void update_hid(uint8_t *report);
    void update_wii(uint8_t *report);
    void update_switch(uint8_t *report);
    void update_ps2(uint8_t *report);
    void update_ps3(uint8_t *report);
    void update_ps4(uint8_t *report);
    void update_ps5(uint8_t *report);
    void update_xinput(uint8_t *report);
    void update_ogxbox(uint8_t *report);
    void update_xboxone(uint8_t *report);
};

class GuitarHeroDrumsButtonMapping : public ButtonMapping
{
public:
    ~GuitarHeroDrumsButtonMapping() {}
    GuitarHeroDrumsButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, uint32_t profile);
    void update_hid(uint8_t *report);
    void update_wii(uint8_t *report);
    void update_switch(uint8_t *report);
    void update_ps2(uint8_t *report);
    void update_ps3(uint8_t *report);
    void update_ps4(uint8_t *report);
    void update_ps5(uint8_t *report);
    void update_xinput(uint8_t *report);
    void update_ogxbox(uint8_t *report);
    void update_xboxone(uint8_t *report);
};
class GuitarHeroDrumsAxisMapping : public AxisMapping
{
public:
    ~GuitarHeroDrumsAxisMapping() {}
    GuitarHeroDrumsAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, uint32_t profile);
    void update_hid(uint8_t *report);
    void update_wii(uint8_t *report);
    void update_switch(uint8_t *report);
    void update_ps2(uint8_t *report);
    void update_ps3(uint8_t *report);
    void update_ps4(uint8_t *report);
    void update_ps5(uint8_t *report);
    void update_xinput(uint8_t *report);
    void update_ogxbox(uint8_t *report);
    void update_xboxone(uint8_t *report);
};

class GuitarHeroGuitarButtonMapping : public ButtonMapping
{
public:
    ~GuitarHeroGuitarButtonMapping() {}
    GuitarHeroGuitarButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, uint32_t profile);
    void update_hid(uint8_t *report);
    void update_wii(uint8_t *report);
    void update_switch(uint8_t *report);
    void update_ps2(uint8_t *report);
    void update_ps3(uint8_t *report);
    void update_ps4(uint8_t *report);
    void update_ps5(uint8_t *report);
    void update_xinput(uint8_t *report);
    void update_ogxbox(uint8_t *report);
    void update_xboxone(uint8_t *report);
};
class GuitarHeroGuitarAxisMapping : public AxisMapping
{
public:
    ~GuitarHeroGuitarAxisMapping() {}
    GuitarHeroGuitarAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, uint32_t profile);
    void update_hid(uint8_t *report);
    void update_wii(uint8_t *report);
    void update_switch(uint8_t *report);
    void update_ps2(uint8_t *report);
    void update_ps3(uint8_t *report);
    void update_ps4(uint8_t *report);
    void update_ps5(uint8_t *report);
    void update_xinput(uint8_t *report);
    void update_ogxbox(uint8_t *report);
    void update_xboxone(uint8_t *report);
    static const uint8_t gh5_slider_mapping[32];
};

class DJHTurntableButtonMapping : public ButtonMapping
{
public:
    ~DJHTurntableButtonMapping() {}
    DJHTurntableButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, uint32_t profile);
    void update_hid(uint8_t *report);
    void update_wii(uint8_t *report);
    void update_switch(uint8_t *report);
    void update_ps2(uint8_t *report);
    void update_ps3(uint8_t *report);
    void update_ps4(uint8_t *report);
    void update_ps5(uint8_t *report);
    void update_xinput(uint8_t *report);
    void update_ogxbox(uint8_t *report);
    void update_xboxone(uint8_t *report);
};
class DJHTurntableAxisMapping : public AxisMapping
{
public:
    ~DJHTurntableAxisMapping() {}
    DJHTurntableAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, uint32_t profile);
    void update_hid(uint8_t *report);
    void update_wii(uint8_t *report);
    void update_switch(uint8_t *report);
    void update_ps2(uint8_t *report);
    void update_ps3(uint8_t *report);
    void update_ps4(uint8_t *report);
    void update_ps5(uint8_t *report);
    void update_xinput(uint8_t *report);
    void update_ogxbox(uint8_t *report);
    void update_xboxone(uint8_t *report);
};

class RockBandGuitarButtonMapping : public ButtonMapping
{
public:
    ~RockBandGuitarButtonMapping() {}
    RockBandGuitarButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, uint32_t profile);
    void update_hid(uint8_t *report);
    void update_wii(uint8_t *report);
    void update_switch(uint8_t *report);
    void update_ps2(uint8_t *report);
    void update_ps3(uint8_t *report);
    void update_ps4(uint8_t *report);
    void update_ps5(uint8_t *report);
    void update_xinput(uint8_t *report);
    void update_ogxbox(uint8_t *report);
    void update_xboxone(uint8_t *report);
};
class RockBandGuitarAxisMapping : public AxisMapping
{
public:
    ~RockBandGuitarAxisMapping() {}
    RockBandGuitarAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, uint32_t profile);
    void update_hid(uint8_t *report);
    void update_wii(uint8_t *report);
    void update_switch(uint8_t *report);
    void update_ps2(uint8_t *report);
    void update_ps3(uint8_t *report);
    void update_ps4(uint8_t *report);
    void update_ps5(uint8_t *report);
    void update_xinput(uint8_t *report);
    void update_ogxbox(uint8_t *report);
    void update_xboxone(uint8_t *report);
};

class GamepadButtonMapping : public ButtonMapping
{
public:
    ~GamepadButtonMapping() {}
    GamepadButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, uint32_t profile);
    void update_hid(uint8_t *report);
    void update_wii(uint8_t *report);
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
class KeyboardManiaButtonMapping : public ButtonMapping
{
public:
    ~KeyboardManiaButtonMapping() {}
    KeyboardManiaButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, uint32_t profile);
    void update_hid(uint8_t *report);
    void update_wii(uint8_t *report);
    void update_switch(uint8_t *report);
    void update_ps2(uint8_t *report);
    void update_ps3(uint8_t *report);
    void update_ps4(uint8_t *report);
    void update_ps5(uint8_t *report);
    void update_xinput(uint8_t *report);
    void update_ogxbox(uint8_t *report);
    void update_xboxone(uint8_t *report);
};
class KeyboardManiaAxisMapping : public AxisMapping
{
public:
    ~KeyboardManiaAxisMapping() {}
    KeyboardManiaAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, uint32_t profile);
    void update_hid(uint8_t *report);
    void update_wii(uint8_t *report);
    void update_switch(uint8_t *report);
    void update_ps2(uint8_t *report);
    void update_ps3(uint8_t *report);
    void update_ps4(uint8_t *report);
    void update_ps5(uint8_t *report);
    void update_xinput(uint8_t *report);
    void update_ogxbox(uint8_t *report);
    void update_xboxone(uint8_t *report);
};
class BeatManiaButtonMapping : public ButtonMapping
{
public:
    ~BeatManiaButtonMapping() {}
    BeatManiaButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, uint32_t profile);
    void update_hid(uint8_t *report);
    void update_wii(uint8_t *report);
    void update_switch(uint8_t *report);
    void update_ps2(uint8_t *report);
    void update_ps3(uint8_t *report);
    void update_ps4(uint8_t *report);
    void update_ps5(uint8_t *report);
    void update_xinput(uint8_t *report);
    void update_ogxbox(uint8_t *report);
    void update_xboxone(uint8_t *report);
};
class BeatManiaAxisMapping : public AxisMapping
{
public:
    ~BeatManiaAxisMapping() {}
    BeatManiaAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, uint32_t profile);
    void update_hid(uint8_t *report);
    void update_wii(uint8_t *report);
    void update_switch(uint8_t *report);
    void update_ps2(uint8_t *report);
    void update_ps3(uint8_t *report);
    void update_ps4(uint8_t *report);
    void update_ps5(uint8_t *report);
    void update_xinput(uint8_t *report);
    void update_ogxbox(uint8_t *report);
    void update_xboxone(uint8_t *report);
};
class DrumManiaButtonMapping : public ButtonMapping
{
public:
    ~DrumManiaButtonMapping() {}
    DrumManiaButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, uint32_t profile);
    void update_hid(uint8_t *report);
    void update_wii(uint8_t *report);
    void update_switch(uint8_t *report);
    void update_ps2(uint8_t *report);
    void update_ps3(uint8_t *report);
    void update_ps4(uint8_t *report);
    void update_ps5(uint8_t *report);
    void update_xinput(uint8_t *report);
    void update_ogxbox(uint8_t *report);
    void update_xboxone(uint8_t *report);
};
class DrumManiaAxisMapping : public AxisMapping
{
public:
    ~DrumManiaAxisMapping() {}
    DrumManiaAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, uint32_t profile);
    void update_hid(uint8_t *report);
    void update_wii(uint8_t *report);
    void update_switch(uint8_t *report);
    void update_ps2(uint8_t *report);
    void update_ps3(uint8_t *report);
    void update_ps4(uint8_t *report);
    void update_ps5(uint8_t *report);
    void update_xinput(uint8_t *report);
    void update_ogxbox(uint8_t *report);
    void update_xboxone(uint8_t *report);
};
class SVDXButtonMapping : public ButtonMapping
{
public:
    ~SVDXButtonMapping() {}
    SVDXButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, uint32_t profile);
    void update_hid(uint8_t *report);
    void update_wii(uint8_t *report);
    void update_switch(uint8_t *report);
    void update_ps2(uint8_t *report);
    void update_ps3(uint8_t *report);
    void update_ps4(uint8_t *report);
    void update_ps5(uint8_t *report);
    void update_xinput(uint8_t *report);
    void update_ogxbox(uint8_t *report);
    void update_xboxone(uint8_t *report);
};
class SVDXAxisMapping : public AxisMapping
{
public:
    ~SVDXAxisMapping() {}
    SVDXAxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, uint32_t profile);
    void update_hid(uint8_t *report);
    void update_wii(uint8_t *report);
    void update_switch(uint8_t *report);
    void update_ps2(uint8_t *report);
    void update_ps3(uint8_t *report);
    void update_ps4(uint8_t *report);
    void update_ps5(uint8_t *report);
    void update_xinput(uint8_t *report);
    void update_ogxbox(uint8_t *report);
    void update_xboxone(uint8_t *report);
};

class ActivationTrigger
{
public:
    ActivationTrigger(uint32_t profile_id) : m_profile_id(profile_id) {}
    ~ActivationTrigger() {}
    virtual void update(bool tool_closed) = 0;

protected:
    uint32_t m_profile_id;
};
class InputActivationTrigger : public ActivationTrigger
{
public:
    InputActivationTrigger(proto_InputActivationTrigger activation_trigger, std::unique_ptr<Input> input, uint32_t profile_id);
    ~InputActivationTrigger() {}
    void update(bool tool_closed);

protected:
    proto_InputActivationTrigger m_activation_trigger;
    std::unique_ptr<Input> m_input;
};
extern const uint8_t gh5_mapping[32];
extern const uint8_t dpad_bindings[11];