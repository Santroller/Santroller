#pragma once
#include <stdint.h>

// Sending MIDI as an instrument. Mappings fill in a MidiState each report, and MidiOutput works out
// the messages that bring whatever is listening up to date with it. Transports (USB, serial, BLE) only
// wrap those messages up, so they all behave the same.

#define MIDI_STATUS_NOTE_OFF 0x80
#define MIDI_STATUS_NOTE_ON 0x90
#define MIDI_STATUS_CONTROL_CHANGE 0xB0
#define MIDI_STATUS_PITCH_BEND 0xE0
#define MIDI_OUTPUT_CHANNELS 16
#define MIDI_OUTPUT_PITCH_BEND_CENTER 0x2000

// A channel voice message. Everything MidiOutput sends is three bytes long.
struct MidiMessage
{
    uint8_t status;
    uint8_t data1;
    uint8_t data2;
    bool operator==(const MidiMessage &other) const
    {
        return status == other.status && data1 == other.data1 && data2 == other.data2;
    }
};

// A message as a USB MIDI event packet: cable number and code index, then the MIDI bytes. For
// channel voice messages the code index is just the top nibble of the status.
static inline void midi_usb_packet(const MidiMessage &message, uint8_t cable, uint8_t *packet)
{
    packet[0] = (uint8_t)((cable << 4) | (message.status >> 4));
    packet[1] = message.status;
    packet[2] = message.data1;
    packet[3] = message.data2;
}

// Scale a 0 - 65535 value to a `bits` wide MIDI value, holding on to `last` (the value sent last) until
// the input moves at least a quarter of a step, and at least 64, past its edges, so a noisy analog input
// doesn't flood the host with changes. The ends, and the centre a stick rests at, go straight through, so
// a released control always lands exactly on them.
static inline uint16_t midi_scale(uint16_t value, uint16_t last, uint8_t bits)
{
    const uint8_t shift = 16 - bits;
    if (value == 0)
    {
        return 0;
    }
    if (value == UINT16_MAX)
    {
        return (uint16_t)((1u << bits) - 1);
    }
    if (value == UINT16_MAX / 2)
    {
        return (uint16_t)(1u << (bits - 1));
    }
    uint16_t scaled = value >> shift;
    if (scaled == last)
    {
        return last;
    }
    int32_t step = 1 << shift;
    int32_t margin = step / 4 > 64 ? step / 4 : 64;
    int32_t low = (int32_t)last * step - margin;
    int32_t high = ((int32_t)last + 1) * step - 1 + margin;
    return (value < low || value > high) ? scaled : last;
}

// A 0 - 65535 hit strength as a note on velocity, which can't be 0 as that would be a note off
static inline uint8_t midi_velocity(uint16_t strength)
{
    uint8_t velocity = strength >> 9;
    return velocity ? velocity : 1;
}

// What the mappings want sent this report. Cleared before the mappings run.
class MidiState
{
public:
    static constexpr uint8_t MAX_NOTES = 32;
    static constexpr uint8_t MAX_CONTROLS = 16;
    struct Note
    {
        uint8_t channel;
        uint8_t note;
        uint8_t velocity;
    };
    struct Control
    {
        uint8_t channel;
        uint8_t control;
        uint8_t value;
    };
    Note notes[MAX_NOTES] = {};
    uint8_t note_count = 0;
    Control controls[MAX_CONTROLS] = {};
    uint8_t control_count = 0;
    uint16_t pitch_bend[MIDI_OUTPUT_CHANNELS] = {};
    // which channels have a pitch bend this report
    uint16_t pitch_bend_mask = 0;

    // A held note. Several mappings can share a note, the hardest hit wins.
    void set_note(uint8_t channel, uint8_t note, uint8_t velocity)
    {
        channel &= 0x0F;
        note &= 0x7F;
        velocity &= 0x7F;
        for (uint8_t i = 0; i < note_count; i++)
        {
            if (notes[i].channel == channel && notes[i].note == note)
            {
                if (velocity > notes[i].velocity)
                {
                    notes[i].velocity = velocity;
                }
                return;
            }
        }
        if (note_count < MAX_NOTES)
        {
            notes[note_count++] = {channel, note, velocity};
        }
    }

    // Several mappings can share a control, the highest value wins
    void set_control(uint8_t channel, uint8_t control, uint8_t value)
    {
        channel &= 0x0F;
        control &= 0x7F;
        value &= 0x7F;
        for (uint8_t i = 0; i < control_count; i++)
        {
            if (controls[i].channel == channel && controls[i].control == control)
            {
                if (value > controls[i].value)
                {
                    controls[i].value = value;
                }
                return;
            }
        }
        if (control_count < MAX_CONTROLS)
        {
            controls[control_count++] = {channel, control, value};
        }
    }

    // 0 - 16383, MIDI_OUTPUT_PITCH_BEND_CENTER at rest. Several mappings can bend a channel, the one
    // furthest from the centre wins.
    void set_pitch_bend(uint8_t channel, uint16_t value)
    {
        channel &= 0x0F;
        if (value > 0x3FFF)
        {
            value = 0x3FFF;
        }
        uint16_t bit = 1u << channel;
        if (!(pitch_bend_mask & bit) || bend_distance(value) > bend_distance(pitch_bend[channel]))
        {
            pitch_bend[channel] = value;
        }
        pitch_bend_mask |= bit;
    }

    // Combine another profile's state into this one
    void merge(const MidiState &other)
    {
        for (uint8_t i = 0; i < other.note_count; i++)
        {
            set_note(other.notes[i].channel, other.notes[i].note, other.notes[i].velocity);
        }
        for (uint8_t i = 0; i < other.control_count; i++)
        {
            set_control(other.controls[i].channel, other.controls[i].control, other.controls[i].value);
        }
        for (uint8_t channel = 0; channel < MIDI_OUTPUT_CHANNELS; channel++)
        {
            if (other.pitch_bend_mask & (1u << channel))
            {
                set_pitch_bend(channel, other.pitch_bend[channel]);
            }
        }
    }

    void clear_all()
    {
        note_count = 0;
        control_count = 0;
        pitch_bend_mask = 0;
    }

private:
    static uint16_t bend_distance(uint16_t value)
    {
        return value > MIDI_OUTPUT_PITCH_BEND_CENTER ? value - MIDI_OUTPUT_PITCH_BEND_CENTER : MIDI_OUTPUT_PITCH_BEND_CENTER - value;
    }
};

// Turns MidiState into messages. Call collect() once per report, then send what pending() gives back
// until it runs out or the transport is full, calling sent() after each one goes out. Nothing is lost
// when the transport is busy, as pending() always works from what was actually sent.
//
// Pads are hit, not held, so a note on waits up to SCAN_US for the hit to peak before it is sent, like
// a drum module does. A hit that is over before that still goes out, with the strongest velocity seen.
// Releasing the input sends the note off, so how long a pad stays held (its debounce) sets how long the
// note lasts. Hitting a note again before its note off went out sends the note off first. A hit that
// still hasn't gone out STALE_US after it ended (the host wasn't reading) is dropped, not played late.
class MidiOutput
{
public:
    static constexpr uint32_t SCAN_US = 2000;
    static constexpr uint32_t STALE_US = 100000;
    // Held notes, plus released ones still waiting to send their note off
    static constexpr uint8_t MAX_TRACKED_NOTES = MidiState::MAX_NOTES * 2;

    void reset()
    {
        m_note_count = 0;
        m_control_count = 0;
        m_pitch_bend_mask = 0;
        m_pitch_bend_sent_mask = 0;
        m_now = 0;
    }

    void collect(const MidiState &state, uint64_t now_us)
    {
        m_now = now_us;
        for (uint8_t i = 0; i < m_note_count; i++)
        {
            m_notes[i].held_now = false;
        }
        for (uint8_t i = 0; i < state.note_count; i++)
        {
            const auto &want = state.notes[i];
            TrackedNote *note = find_note(want.channel, want.note);
            if (!note)
            {
                if (m_note_count >= MAX_TRACKED_NOTES)
                {
                    continue;
                }
                note = &m_notes[m_note_count++];
                *note = {want.channel, want.note, want.velocity, true, true, false, false, now_us, now_us};
                continue;
            }
            note->held_now = true;
            if (!note->held)
            {
                // a new hit before the last one's note off went out
                if (note->sounding)
                {
                    note->retrigger = true;
                }
                note->velocity = want.velocity;
                note->scan_start = now_us;
            }
            else if (!note->on_sent() && want.velocity > note->velocity)
            {
                note->velocity = want.velocity;
            }
        }
        for (uint8_t i = 0; i < m_note_count;)
        {
            TrackedNote &note = m_notes[i];
            if (note.held && !note.held_now)
            {
                note.released = now_us;
            }
            note.held = note.held_now;
            if (!note.held && !note.sounding && now_us - note.released > STALE_US)
            {
                note = m_notes[--m_note_count];
                continue;
            }
            i++;
        }

        for (uint8_t i = 0; i < state.control_count; i++)
        {
            const auto &want = state.controls[i];
            TrackedControl *control = find_control(want.channel, want.control);
            if (!control)
            {
                if (m_control_count >= MidiState::MAX_CONTROLS)
                {
                    continue;
                }
                control = &m_controls[m_control_count++];
                *control = {want.channel, want.control, want.value, 0, false};
            }
            control->value = want.value;
        }

        for (uint8_t channel = 0; channel < MIDI_OUTPUT_CHANNELS; channel++)
        {
            if (state.pitch_bend_mask & (1u << channel))
            {
                m_pitch_bend[channel] = state.pitch_bend[channel];
                m_pitch_bend_mask |= 1u << channel;
            }
        }
    }

    // The next message to send: note offs, then note ons, then controls and pitch bends
    bool pending(MidiMessage &message) const
    {
        for (uint8_t i = 0; i < m_note_count; i++)
        {
            const auto &note = m_notes[i];
            if (note.sounding && (!note.held || note.retrigger))
            {
                message = {(uint8_t)(MIDI_STATUS_NOTE_OFF | note.channel), note.note, 0};
                return true;
            }
        }
        for (uint8_t i = 0; i < m_note_count; i++)
        {
            const auto &note = m_notes[i];
            if (!note.sounding && (!note.held || note.velocity == 0x7F || m_now - note.scan_start >= SCAN_US))
            {
                message = {(uint8_t)(MIDI_STATUS_NOTE_ON | note.channel), note.note, note.velocity};
                return true;
            }
        }
        for (uint8_t i = 0; i < m_control_count; i++)
        {
            const auto &control = m_controls[i];
            if (!control.has_sent || control.value != control.sent_value)
            {
                message = {(uint8_t)(MIDI_STATUS_CONTROL_CHANGE | control.channel), control.control, control.value};
                return true;
            }
        }
        for (uint8_t channel = 0; channel < MIDI_OUTPUT_CHANNELS; channel++)
        {
            uint16_t bit = 1u << channel;
            if ((m_pitch_bend_mask & bit) &&
                (!(m_pitch_bend_sent_mask & bit) || m_pitch_bend[channel] != m_pitch_bend_sent[channel]))
            {
                uint16_t value = m_pitch_bend[channel];
                message = {(uint8_t)(MIDI_STATUS_PITCH_BEND | channel), (uint8_t)(value & 0x7F), (uint8_t)(value >> 7)};
                return true;
            }
        }
        return false;
    }

    // Call once the message pending() gave back has gone out
    void sent(const MidiMessage &message)
    {
        uint8_t type = message.status & 0xF0;
        uint8_t channel = message.status & 0x0F;
        if (type == MIDI_STATUS_NOTE_OFF)
        {
            TrackedNote *note = find_note(channel, message.data1);
            if (!note)
            {
                return;
            }
            note->sounding = false;
            note->retrigger = false;
            if (!note->held)
            {
                *note = m_notes[--m_note_count];
            }
        }
        else if (type == MIDI_STATUS_NOTE_ON)
        {
            TrackedNote *note = find_note(channel, message.data1);
            if (note)
            {
                note->sounding = true;
            }
        }
        else if (type == MIDI_STATUS_CONTROL_CHANGE)
        {
            TrackedControl *control = find_control(channel, message.data1);
            if (control)
            {
                control->sent_value = message.data2;
                control->has_sent = true;
            }
        }
        else if (type == MIDI_STATUS_PITCH_BEND)
        {
            m_pitch_bend_sent[channel] = message.data1 | (message.data2 << 7);
            m_pitch_bend_sent_mask |= 1u << channel;
        }
    }

private:
    struct TrackedNote
    {
        uint8_t channel;
        uint8_t note;
        // strongest velocity seen for the hit that hasn't been sent yet
        uint8_t velocity;
        bool held;
        bool held_now;
        // a note on went out and its note off hasn't yet
        bool sounding;
        // hit again while sounding, so it needs a note off and then a fresh note on
        bool retrigger;
        uint64_t scan_start;
        uint64_t released;
        // whether the current hit's note on has gone out
        bool on_sent() const { return sounding && !retrigger; }
    };
    struct TrackedControl
    {
        uint8_t channel;
        uint8_t control;
        uint8_t value;
        uint8_t sent_value;
        bool has_sent;
    };

    TrackedNote *find_note(uint8_t channel, uint8_t note)
    {
        for (uint8_t i = 0; i < m_note_count; i++)
        {
            if (m_notes[i].channel == channel && m_notes[i].note == note)
            {
                return &m_notes[i];
            }
        }
        return nullptr;
    }

    TrackedControl *find_control(uint8_t channel, uint8_t control)
    {
        for (uint8_t i = 0; i < m_control_count; i++)
        {
            if (m_controls[i].channel == channel && m_controls[i].control == control)
            {
                return &m_controls[i];
            }
        }
        return nullptr;
    }

    TrackedNote m_notes[MAX_TRACKED_NOTES] = {};
    uint8_t m_note_count = 0;
    TrackedControl m_controls[MidiState::MAX_CONTROLS] = {};
    uint8_t m_control_count = 0;
    uint16_t m_pitch_bend[MIDI_OUTPUT_CHANNELS] = {};
    uint16_t m_pitch_bend_sent[MIDI_OUTPUT_CHANNELS] = {};
    uint16_t m_pitch_bend_mask = 0;
    uint16_t m_pitch_bend_sent_mask = 0;
    uint64_t m_now = 0;
};
