#pragma once
#include "leds/base_led.hpp"
#include "input.pb.h"
#include "input/input.hpp"
#include <memory>

class Profile;

class LedMapping
{
public:
    LedMapping(std::unique_ptr<LedMappingDevice> device, std::shared_ptr<Profile> profile, uint32_t id) : m_device(std::move(device)), m_id(id), m_profile(profile) {}
    virtual ~LedMapping() {}
    void off();
    virtual void update(bool full_poll, bool send_events) = 0;
    virtual void reload() = 0;

protected:
    std::unique_ptr<LedMappingDevice> m_device;
    uint32_t m_id;
    std::shared_ptr<Profile> m_profile;
};

class InputLedMapping : public LedMapping
{
public:
    InputLedMapping(std::unique_ptr<LedMappingDevice> device, proto_InputLedMapping mapping, std::unique_ptr<Input> input, std::shared_ptr<Profile> profile, uint32_t id);
    void update(bool full_poll, bool send_events);
    void reload();

protected:
    std::unique_ptr<Input> m_input;
    proto_InputLedMapping m_mapping;

private:
    uint16_t m_last_val = 0;
    uint32_t m_last_poll = 0;
    float m_multiplier = 0;
    uint32_t m_pos = 0;
    uint32_t m_last_increase = 0;
    uint32_t m_last_decay = 0;
};

class PatternLedMapping : public LedMapping
{
public:
    PatternLedMapping(std::unique_ptr<LedMappingDevice> device, proto_PatternLedMapping mapping, std::shared_ptr<Profile> profile, uint32_t id);
    void update(bool full_poll, bool send_events);
    void reload();

protected:
    proto_PatternLedMapping m_mapping;
    uint32_t m_pos = 0;
    bool m_dir = false;
    uint32_t m_next_poll = 0;
    uint32_t m_speed = 0;
    uint32_t m_brightness = 0;
};

class StaticLedMapping : public LedMapping
{
public:
    StaticLedMapping(std::unique_ptr<LedMappingDevice> device, proto_StaticLedMapping mapping, std::shared_ptr<Profile> profile, uint32_t id) : LedMapping(std::move(device), profile, id), m_mapping(mapping) {}
    void update(bool full_poll, bool send_events);
    void reload();

protected:
    proto_StaticLedMapping m_mapping;
};
