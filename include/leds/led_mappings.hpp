#pragma once
#include "leds/base_led.hpp"
#include "input.pb.h"
#include "input/input.hpp"
#include <memory>

class Profile;

// What the game has reported through the Santroller LED commands
struct GameFeedback
{
    uint8_t star_power_fill = 0;
    bool star_power_active = false;
    uint8_t multiplier = 0;
    bool solo = false;
    bool note_miss = false;
    uint8_t note_hits = 0;
};

class LedMapping
{
public:
    LedMapping(std::unique_ptr<LedMappingDevice> device, Profile *profile, uint32_t id) : m_device(std::move(device)), m_id(id), m_profile(profile) {}
    LedMapping(std::unique_ptr<LedMappingDevice> device, const std::shared_ptr<Profile> &profile, uint32_t id) : m_device(std::move(device)), m_id(id), m_profile(profile.get()) {}
    virtual ~LedMapping() {}
    void off();
    virtual void update(bool full_poll, bool send_events) = 0;
    virtual void reload() = 0;
    virtual void set_rumble(uint8_t left, uint8_t right) {};
    virtual void set_player_led(uint8_t player) {};
    virtual void set_lightbar(uint8_t r, uint8_t g, uint8_t b) {};
    virtual void set_euphoria_led(uint8_t val) {};
    virtual void set_stagekit_led(uint8_t fog, uint8_t strobe, uint8_t blue, uint8_t green, uint8_t yellow, uint8_t red) {};
    virtual void set_game_feedback(const GameFeedback &feedback) {};
    virtual void set_keyboard_leds(uint8_t leds) {};

protected:
    std::unique_ptr<LedMappingDevice> m_device;
    uint32_t m_id;
    Profile *m_profile;
};
class RumbleLedMapping : public LedMapping
{
public:
    RumbleLedMapping(std::unique_ptr<LedMappingDevice> device, proto_RumbleLedMapping mapping, std::shared_ptr<Profile> profile, uint32_t id);
    void update(bool full_poll, bool send_events);
    void reload();
    void set_rumble(uint8_t left, uint8_t right) override;

protected:
    proto_RumbleLedMapping m_mapping;
};
class StageKitLedMapping : public LedMapping
{
public:
    StageKitLedMapping(std::unique_ptr<LedMappingDevice> device, proto_StageKitLedMapping mapping, std::shared_ptr<Profile> profile, uint32_t id);
    void update(bool full_poll, bool send_events);
    void reload();
    void set_stagekit_led(uint8_t fog, uint8_t strobe, uint8_t blue, uint8_t green, uint8_t yellow, uint8_t red) override;

protected:
    proto_StageKitLedMapping m_mapping;
};

class PlaystationLedMapping : public LedMapping
{
public:
    PlaystationLedMapping(std::unique_ptr<LedMappingDevice> device, proto_PlaystationLedMapping mapping, std::shared_ptr<Profile> profile, uint32_t id);
    void update(bool full_poll, bool send_events);
    void reload();
    void set_lightbar(uint8_t r, uint8_t g, uint8_t b) override;

protected:
    proto_PlaystationLedMapping m_mapping;
};
class EuphoriaLedMapping : public LedMapping
{
public:
    EuphoriaLedMapping(std::unique_ptr<LedMappingDevice> device, proto_EuphoriaLedMapping mapping, std::shared_ptr<Profile> profile, uint32_t id);
    void update(bool full_poll, bool send_events);
    void reload();
    void set_euphoria_led(uint8_t val) override;

protected:
    proto_EuphoriaLedMapping m_mapping;
};
class PlayerLedMapping : public LedMapping
{
public:
    PlayerLedMapping(std::unique_ptr<LedMappingDevice> device, proto_PlayerLedMapping mapping, std::shared_ptr<Profile> profile, uint32_t id);
    void update(bool full_poll, bool send_events);
    void reload();
    void set_player_led(uint8_t player) override;

protected:
    proto_PlayerLedMapping m_mapping;
};
class KeyboardLedMapping : public LedMapping
{
public:
    KeyboardLedMapping(std::unique_ptr<LedMappingDevice> device, proto_KeyboardLedMapping mapping, std::shared_ptr<Profile> profile, uint32_t id) : LedMapping(std::move(device), profile, id), m_mapping(mapping) {}
    void update(bool full_poll, bool send_events) {}
    void reload() {}
    void set_keyboard_leds(uint8_t leds) override;

protected:
    proto_KeyboardLedMapping m_mapping;
};

class GameFeedbackLedMapping : public LedMapping
{
public:
    GameFeedbackLedMapping(std::unique_ptr<LedMappingDevice> device, proto_GameFeedbackLedMapping mapping, std::shared_ptr<Profile> profile, uint32_t id) : LedMapping(std::move(device), profile, id), m_mapping(mapping) {}
    void update(bool full_poll, bool send_events) {}
    void reload() {}
    void set_game_feedback(const GameFeedback &feedback) override;

protected:
    proto_GameFeedbackLedMapping m_mapping;
};

// Shows the controller's own state, so it is checked every update rather than pushed by the host
class StatusLedMapping : public LedMapping
{
public:
    StatusLedMapping(std::unique_ptr<LedMappingDevice> device, proto_StatusLedMapping mapping, std::shared_ptr<Profile> profile, uint32_t id) : LedMapping(std::move(device), profile, id), m_mapping(mapping) {}
    void update(bool full_poll, bool send_events);
    void reload() {}

protected:
    proto_StatusLedMapping m_mapping;
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
    // the input range that drives the LED from off to fully on, min can be above max to invert it
    int32_t m_min = 0;
    int32_t m_max = UINT16_MAX;
    uint16_t scale(uint16_t raw) const;
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
