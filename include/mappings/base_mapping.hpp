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
    Mapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, Profile *profile) : m_mapping(mapping), m_id(id), m_profile(profile), m_input(std::move(input)) {}
    Mapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, const std::shared_ptr<Profile> &profile) : Mapping(mapping, std::move(input), id, profile.get()) {}
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
    uint16_t id() const { return m_id; }
    Input* get_input() const { return m_input.get(); }
    void add_masked_mapping(Mapping* mapping) {
        for (auto *m : m_masked_mappings) {
            if (m == mapping) return;
        }
        m_masked_mappings.push_back(mapping);
    }
    void clear_masked_mappings() { m_masked_mappings.clear(); }
    void mask_by_shortcut() {
        m_suppressed = true;
        m_waiting_for_release = true;
    }
    bool is_suppressed() const { return m_suppressed || m_waiting_for_release; }

protected:
    proto_Mapping m_mapping;
    uint16_t m_id;
    Profile *m_profile;
    uint32_t m_last_value_raw = 0;
    uint32_t m_last_sent_value = 0;
    uint32_t m_last_sent_calibrated_value = 0;
    uint32_t m_last_send = 0;
    std::unique_ptr<Input> m_input;
    std::vector<Mapping*> m_masked_mappings;
    bool m_suppressed = false;
    bool m_waiting_for_release = false;
};

class ButtonMapping : public Mapping
{
public:
    ~ButtonMapping() {}
    ButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, Profile *profile) : Mapping(mapping, std::move(input), id, profile) {}
    ButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, const std::shared_ptr<Profile> &profile) : Mapping(mapping, std::move(input), id, profile) {}
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
    AxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, Profile *profile, bool trigger) : Mapping(mapping, std::move(input), id, profile), m_trigger(trigger) {}
    AxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, const std::shared_ptr<Profile> &profile, bool trigger) : Mapping(mapping, std::move(input), id, profile), m_trigger(trigger) {}
    void update(bool full_poll, bool send_events);

protected:
    uint32_t m_calibrated_value = 0;
    bool m_centered = false;
    bool m_trigger;
    uint32_t m_last_value = 0;
    uint64_t m_last_poll = 0;
};
