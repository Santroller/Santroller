#pragma once
#include <stdint.h>
#include <memory>
#include "input/input.hpp"
#include "profiles/profile.hpp"
#include "input.pb.h"
#include "config.pb.h"
#include "wiimote.h"
#include "mappings/analog_filter.hpp"

struct MappingConfig
{
    explicit MappingConfig(const proto_Mapping &source)
        : mapping(source.mapping),
          inverted(source.inverted),
          has_trigger(source.has_trigger),
          trigger(source.trigger),
          has_pressed(source.has_pressed),
          pressed(source.pressed),
          has_released(source.has_released),
          released(source.released),
          min(source.min),
          max(source.max),
          center(source.center),
          deadzone(source.deadzone),
          triggerValue(source.triggerValue),
          has_debounce(source.has_debounce100us || source.has_debounce),
          debounce_us(source.has_debounce100us ? source.debounce100us * 100 : source.debounce * 1000),
          debounce_mode(source.has_debounceMode ? source.debounceMode : DebounceRelease),
          maxTriggerValue(source.maxTriggerValue),
          has_peakBased(source.has_peakBased),
          peakBased(source.peakBased),
          section(source.has_section ? source.section : AxisSectionFull),
          section_start(UINT16_MAX / 2),
          section_end(UINT16_MAX)
    {
        if (section == AxisSectionNegative)
        {
            section_end = 0;
        }
        else if (section == AxisSectionCustom)
        {
            section_start = source.has_sectionStart ? source.sectionStart : 0;
            section_end = source.has_sectionEnd ? source.sectionEnd : UINT16_MAX;
        }
    }

    proto_Output mapping;
    bool inverted;
    bool has_trigger;
    proto_AnalogToDigitalTriggerType trigger;
    bool has_pressed;
    int32_t pressed;
    bool has_released;
    int32_t released;
    int32_t min;
    int32_t max;
    int32_t center;
    int32_t deadzone;
    int32_t triggerValue;
    bool has_debounce;
    uint32_t debounce_us;
    DebounceMode debounce_mode;
    int32_t maxTriggerValue;
    bool has_peakBased;
    bool peakBased;
    AxisSection section;
    // the axis values a section moves between, from rest to fully pressed
    uint32_t section_start;
    uint32_t section_end;
};

class ButtonMapping;

class Mapping
{
public:
    Mapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, Profile *profile) : m_mapping(mapping), m_id(id), m_profile(profile), m_input(std::move(input))
    {
        // only mappings that use them pay for the filter state. Everything but a stick rests at min,
        // sticks move it to their center once they know they are one.
        if (mapping.has_filter)
        {
            m_filter = std::make_unique<AnalogFilter>(mapping.filter, mapping.min);
            if (!m_filter->enabled())
            {
                m_filter.reset();
            }
        }
    }
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
    virtual void update_gamecube(uint8_t *report) { (void)report; }
    virtual void update_n64(uint8_t *report) { (void)report; }
    virtual void update_snes(uint8_t *report) { (void)report; }
    virtual void update_nes(uint8_t *report) { (void)report; }
    virtual void update_ps3(uint8_t *report) { (void)report; }
    virtual void update_ps4(uint8_t *report) { (void)report; }
    virtual void update_ps5(uint8_t *report) { (void)report; }
    virtual void update_xinput(uint8_t *report) { (void)report; }
    virtual void update_pdloader(uint8_t *report) { (void)report; }
    virtual void update_ogxbox(uint8_t *report) { (void)report; }
    virtual void update_xboxone(uint8_t *report) { (void)report; }
    // Whether this mapping is currently held as a button, used to wake a suspended host
    virtual bool wake_pressed() const { return false; }
    virtual ButtonMapping *as_button_mapping() { return nullptr; }
    void update_digital(bool full_poll);
    uint16_t sample_ui_event();
    uint16_t calibrate(float val, float max, float min, float deadzone, float center, bool trigger);
    // The input's analog value, after any configured filters
    uint16_t tick_filtered_analog();
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
    // Mask the mappings this shortcut covers while its chord (or, without a shortcut input,
    // pressed) is held. queued_only limits it to queued fret / strum mappings
    void mask_shortcut_members(bool pressed, bool queued_only = false);

protected:
    MappingConfig m_mapping;
    std::unique_ptr<AnalogFilter> m_filter;
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
    uint16_t m_ui_event_value = 0;
    uint32_t m_ui_event_time = 0;
};

class ButtonMapping : public Mapping
{
public:
    ~ButtonMapping() {}
    ButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, Profile *profile) : Mapping(mapping, std::move(input), id, profile)
    {
        if (mapping.has_rapidTrigger)
        {
            m_rapid_trigger = std::make_unique<RapidTrigger>(mapping.rapidTrigger, mapping.triggerValue);
        }
    }
    ButtonMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, const std::shared_ptr<Profile> &profile) : ButtonMapping(mapping, std::move(input), id, profile.get()) {}
    void update(bool full_poll, bool send_events);
    // Read and debounce the input without touching the value outputs report
    void sample(bool full_poll, bool send_events);
    bool wake_pressed() const override { return m_live_value; }
    bool live_value() const { return m_live_value; }
    int8_t queue_bit() const { return m_queue_bit; }
    void set_queue_bit(int8_t bit) { m_queue_bit = bit; }
    int8_t strum_bit() const { return m_strum_bit; }
    void set_strum_bit(int8_t bit) { m_strum_bit = bit; }
    ButtonMapping *as_button_mapping() override { return this; }

protected:
    // What outputs report: the live value, or the queued state for queued mappings
    bool m_last_value = false;
    uint16_t m_last_pressure = 0;
    bool m_live_value = false;
    uint16_t m_live_pressure = 0;
    int8_t m_queue_bit = -1;
    int8_t m_strum_bit = -1;
    bool m_last_sent_value = false;
    uint16_t m_last_sent_pressure = 0;
    bool m_calibrated_value = false;
    // with release debounce, when the input was last pressed. With press debounce, when the
    // current press started
    uint64_t m_last_poll = 0;
    bool m_was_pressed = false;
    std::unique_ptr<RapidTrigger> m_rapid_trigger;
};

// Runs an action on the controller itself when pressed. Actions are checked every loop from
// the profile rather than when a report is built, so they work even when the host isn't
// listening, which is exactly when restarting the device stack is needed.
class ActionMapping : public ButtonMapping
{
public:
    ~ActionMapping() {}
    ActionMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, const std::shared_ptr<Profile> &profile) : ButtonMapping(mapping, std::move(input), id, profile) {}
    void update_action();

private:
    // Starts as held, so a button still held after a restart (or at boot) needs releasing first
    bool m_was_pressed = true;
};

class AxisMapping : public Mapping
{
public:
    ~AxisMapping() {}
    AxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, Profile *profile, bool trigger) : Mapping(mapping, std::move(input), id, profile), m_trigger(trigger)
    {
        // sections calibrate like a trigger, so they rest at min too
        if (m_filter && !trigger && m_mapping.section == AxisSectionFull)
        {
            m_filter->set_rest(m_mapping.center);
        }
    }
    AxisMapping(proto_Mapping mapping, std::unique_ptr<Input> input, uint16_t id, const std::shared_ptr<Profile> &profile, bool trigger) : AxisMapping(mapping, std::move(input), id, profile.get(), trigger) {}
    void update(bool full_poll, bool send_events);

protected:
    uint32_t m_calibrated_value = 0;
    bool m_centered = false;
    bool m_trigger;
    uint32_t m_last_value = 0;
    uint64_t m_last_poll = 0;
};
