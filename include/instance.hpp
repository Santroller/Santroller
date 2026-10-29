#pragma once
#include <map>
#include <vector>
#include <memory>
#include "mappings/mapping.hpp"
#include "devices/base.hpp"
#include "input/gpio.hpp"
#include "input/input.hpp"
#include "leds/leds.hpp"
#include "profiles/profile.hpp"

#include "config.pb.h"
#include <stdio.h>

class Instance
{
public:
    virtual ~Instance() {}
    virtual void initialize() = 0;
    virtual void deinitialize() {}
    virtual bool is_bluetooth() const { return false; }
    // What a MIDI instrument played on its last process, for MIDI out jacks to send as well
    virtual const MidiState *midi_output_state() const { return nullptr; }
    virtual void process(bool full_poll, bool send_events) = 0;
    // While the USB host is suspended, ProfileManager calls process_suspended instead of process for
    // instances that sleep along with it. Returns whether a button that wakes the host is held.
    virtual bool suspends_with_host() const { return false; }
    virtual bool process_suspended(bool full_poll, bool send_events)
    {
        process(full_poll, send_events);
        return false;
    }
    // The host is in standby without suspending the bus (a 360 keeps polling its pads)
    virtual bool host_in_standby() const { return false; }
    SubType subtype;
    ConsoleMode mode;
    bool xinput_on_windows = 0;
    bool invert_y_axis_hid = 0;
    bool supports_ps4 = 0;
    std::vector<std::shared_ptr<Profile>> profiles;
    uint8_t rumble_left = 0;
    uint8_t rumble_right = 0;
    uint8_t player_led = 1;
    bool player_led_set = false;
    uint8_t euphoria_led = 0;
    uint8_t lightbar_red = 0;
    uint8_t lightbar_green = 0;
    uint8_t lightbar_blue = 0;
    uint8_t stagekit_param = 0;
    uint8_t stagekit_command = 0;
    uint8_t stagekit_fog = 0;
    uint16_t stagekit_strobe_speed = 0;
    uint8_t stagekit_strobe = 0;
    uint8_t stagekit_blue = 0;
    uint8_t stagekit_green = 0;
    uint8_t stagekit_yellow = 0;
    uint8_t stagekit_red = 0;
    uint32_t stagekit_last_strobe = 0;
    uint8_t capabilities = 0;
    GameFeedback game_feedback;
    uint8_t keyboard_leds = 0;

    void set_rumble(uint8_t left, uint8_t right);
    void set_player_led(uint8_t player);
    void set_lightbar(uint8_t r, uint8_t g, uint8_t b);
    void set_euphoria_led(uint8_t val);
    void process_stagekit_command(uint8_t command, uint8_t param);
    // The batched Santroller LED report (0x5B), starting at its type byte
    void process_feedback_report(const uint8_t *data, uint16_t len);
    void set_keyboard_leds(uint8_t leds);
    void update_stagekit();
    void update_feedback(bool force = false);
    void update_capabilities();
};