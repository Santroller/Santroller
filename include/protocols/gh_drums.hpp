#pragma once
#include <stdint.h>

// Guitar Hero kits (PS3 and 360) pass the MIDI note-on for their hi-hat pedal on in their reports, as
// channel 10 note 100 (PlasticBand: 5-Lane Drums). The pedal normally comes with a kick velocity
// too, but a report with only the note is still a kick.
inline uint8_t gh_drum_hi_hat_velocity(uint8_t status, uint8_t note, uint8_t velocity)
{
    return status == 0x99 && note == 100 ? velocity & 0x7F : 0;
}

// The kick velocity, falling back to the hi-hat note's when the kit didn't send one
inline uint8_t gh_drum_kick_velocity(uint8_t kick, uint8_t status, uint8_t note, uint8_t velocity)
{
    return kick ? kick : gh_drum_hi_hat_velocity(status, note, velocity);
}

// A pedal in a GHWT brain's hi-hat port (eg a Rock Band kick pedal) doesn't send one note per press.
// Measured on a Wii kit, a press sends hi-hat note 100 every ~50ms (50 - 56) while it's down, three
// or four times for a tap, and (in every capture that caught the start of the press) a kick 130 -
// 215ms before the first of them. With
// both played as kicks, each of those was a kick of its own. So only the first note 100 of a run is a
// press, and not even that when the run follows a kick.
class GhHiHatFilter
{
public:
    // Notes 100 closer together than this are one press
    static constexpr uint32_t REPEAT_MS = 90;
    // A run of note 100s starting this soon after a kick is the same press as the kick
    static constexpr uint32_t AFTER_KICK_MS = 300;

    void reset()
    {
        m_has_hi_hat = false;
        m_has_kick = false;
    }
    void kick(uint32_t now_ms)
    {
        m_has_kick = true;
        m_last_kick_ms = now_ms;
    }
    // Whether a hi-hat note now is a press of its own
    bool hi_hat(uint32_t now_ms)
    {
        const bool repeat = m_has_hi_hat && now_ms - m_last_hi_hat_ms < REPEAT_MS;
        m_has_hi_hat = true;
        m_last_hi_hat_ms = now_ms;
        return !repeat && !(m_has_kick && now_ms - m_last_kick_ms < AFTER_KICK_MS);
    }
    // For a kit that reports its hi-hat note alongside its kick velocity: blanks the note (its status
    // byte) when it isn't a press of its own, so gh_drum_kick_velocity only falls back to new ones
    void filter_report(uint8_t kick_velocity, uint8_t &status, uint8_t note, uint8_t velocity, uint32_t now_ms)
    {
        if (kick_velocity)
        {
            kick(now_ms);
        }
        if (gh_drum_hi_hat_velocity(status, note, velocity) && !hi_hat(now_ms))
        {
            status = 0;
        }
    }

private:
    bool m_has_hi_hat = false;
    uint32_t m_last_hi_hat_ms = 0;
    bool m_has_kick = false;
    uint32_t m_last_kick_ms = 0;
};
