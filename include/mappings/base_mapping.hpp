#pragma once
#include <stdint.h>
#include <memory>
#include "input/input.hpp"
#include "profiles/profile.hpp"
#include "input.pb.h"
#include "config.pb.h"
#include "wiimote.h"

class Mapping
{
public:
    Mapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : m_mapping(mapping), m_id(id), m_profile(profile), m_input(std::move(input)) {}
    virtual ~Mapping() {}
    inline void reload()
    {
        m_input->setup();
    }
    virtual void update(bool full_poll, bool send_events) = 0;
    virtual void update_hid(uint8_t *report) { (void)report; }
    virtual void update_wii(uint8_t format, uint8_t *buf) { (void)format; (void)buf; }
    virtual void update_wiimote_core(wiimote_buttons *buttons) { (void)buttons; }
    virtual void update_switch(uint8_t *report) { (void)report; }
    virtual void update_ps2(uint8_t *report) { (void)report; }
    virtual void update_ps3(uint8_t *report) { (void)report; }
    virtual void update_ps4(uint8_t *report) { (void)report; }
    virtual void update_ps5(uint8_t *report) { (void)report; }
    virtual void update_xinput(uint8_t *report) { (void)report; }
    virtual void update_ogxbox(uint8_t *report) { (void)report; }
    virtual void update_xboxone(uint8_t *report) { (void)report; }
    void update_digital(bool full_poll);
    uint16_t calibrate(float val, float max, float min, float deadzone, float center, bool trigger);

protected:
    proto_Mapping m_mapping;
    uint16_t m_id;
    std::shared_ptr<Profile> m_profile;
    uint32_t m_last_value_raw = 0;
    uint32_t m_last_sent_value = 0;
    uint32_t m_last_sent_calibrated_value = 0;
    uint32_t m_last_send = 0;
    std::unique_ptr<Input> m_input;
};

class ButtonMapping : public Mapping
{
public:
    ~ButtonMapping() {}
    ButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : Mapping(mapping, std::move(input), id, profile) {}
    void update(bool full_poll, bool send_events);

protected:
    bool m_last_value = false;
    bool m_last_sent_value = false;
    bool m_calibrated_value = false;
    uint64_t m_last_poll = 0;
};

class AxisMapping : public Mapping
{
public:
    ~AxisMapping() {}
    AxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile, bool trigger) : Mapping(mapping, std::move(input), id, profile), m_trigger(trigger) {}
    void update(bool full_poll, bool send_events);

protected:
    uint32_t m_calibrated_value = 0;
    bool m_centered = false;
    bool m_trigger;
    uint32_t m_last_value = 0;
    uint64_t m_last_poll = 0;
};
