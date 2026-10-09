#pragma once
#include <stdint.h>
#include <string.h>
#include "class/midi/midi.h"
#include "protocols/controller_reports.hpp"
#include "input_enums.pb.h"

// Where a 14 bit MIDI pitch wheel rests
#define MIDI_PITCH_BEND_CENTER 0x2000

#define MIDI_CONTROL_COMMAND_MOD_WHEEL 1
#define MIDI_CONTROL_COMMAND_SUSTAIN_PEDAL 64
#define MIDI_CHANNEL_PROGUITAR_SQUIER 16
#define MIDI_CHANNEL_PROGUITAR_MUSTANG 17
#define MIDI_SYSEX_ID_PROGUITAR_SQUIER 0x08
#define MIDI_SYSEX_ID_PROGUITAR_MUSTANG 0x0A
#define USB_PACKET_SIZE 4
typedef struct
{
    unsigned int status;
    unsigned int pos;
    unsigned int actual_size;
    uint8_t data[32];
    bool sysex_in_progress;
} cable_state_t;

// Parses incoming MIDI and keeps track of the notes, control changes, pitch bends and Pro Guitar state it
// has seen. Data is either 4 byte USB MIDI event packets (cable number + CIN, then the MIDI bytes) or a
// plain MIDI byte stream (serial and BLE MIDI) that can use running status.
class MidiInput
{
public:
    MidiInput(bool usbPackets) : usbPackets(usbPackets)
    {
        memset(midiNoteEvents, 0, sizeof(midiNoteEvents));
        // the pitch wheel rests in the middle of its 14 bit range
        for (auto &pitch : midiPitchWheel)
        {
            pitch = MIDI_PITCH_BEND_CENTER;
        }
        memset(midiFrets, 0, sizeof(midiFrets));
        memset(midiStringVelocities, 0, sizeof(midiStringVelocities));
        memset(&midiButtons, 0, sizeof(midiButtons));
        memset(seenChannels, 0, sizeof(seenChannels));
        // default to neutral
        midiButtons.dpad = 8;
        set_max_cables(1);
    }
    ~MidiInput()
    {
        delete[] cable_status;
        cable_status = nullptr;
        for (int i = 0; i < 16; i++)
        {
            delete[] midiControlChanges[i];
            midiControlChanges[i] = nullptr;
            delete[] midiNoteVelocity[i];
            midiNoteVelocity[i] = nullptr;
        }
    }
    MidiInput(const MidiInput &) = delete;
    MidiInput &operator=(const MidiInput &) = delete;

    // Whether data arrives as 4 byte USB MIDI packets rather than a plain MIDI byte stream
    void set_usb_packets(bool usb_packets) { usbPackets = usb_packets; }
    // USB MIDI packets for cables past the last one are treated as cable 0
    void set_max_cables(uint8_t max_cables)
    {
        if (cable_status)
        {
            delete[] cable_status;
        }
        m_max_cables = max_cables ? max_cables : 1;
        cable_status = new cable_state_t[m_max_cables]();
    }

    // Parse everything available in stream, which provides:
    //   bool peek(uint8_t &byte), uint32_t peek_n(void *buf, uint16_t len), uint32_t read(void *buf, uint32_t len)
    // on_message(data, size) is called for each complete message before it is applied, and
    // mark_channel_seen(channel) for each channel (or Pro Guitar pseudo channel) that sends something.
    template <typename Stream, typename OnMessage, typename MarkChannelSeen>
    void parse(Stream &stream, OnMessage &&on_message, MarkChannelSeen &&mark_channel_seen)
    {
        if (!usbPackets)
        {
            uint8_t byte;
            while (stream.read(&byte, 1))
            {
                parse_byte(&cable_status[0], byte, on_message, mark_channel_seen);
            }
            return;
        }
        // USB MIDI only ever sends whole packets, so leave a partial one until the rest arrives
        uint8_t usb_packet[USB_PACKET_SIZE];
        while (stream.peek_n(usb_packet, USB_PACKET_SIZE) == USB_PACKET_SIZE)
        {
            stream.read(usb_packet, USB_PACKET_SIZE);
            uint8_t cable_num = (usb_packet[0] >> 4) & 0x0f;
            cable_state_t *cable_state = (cable_num < m_max_cables) ? &cable_status[cable_num] : &cable_status[0];
            // the Code Index Number says how many of the three bytes are MIDI, the rest is padding.
            // This also skips empty (all zero) packets, as CIN 0 is reserved.
            uint8_t len = usb_midi_cin_size(usb_packet[0] & 0x0f);
            for (uint8_t i = 0; i < len; i++)
            {
                parse_byte(cable_state, usb_packet[i + 1], on_message, mark_channel_seen);
            }
        }
    }

    bool consume_midi_note_event(uint8_t channel, uint8_t note, uint16_t &sequence, uint16_t &velocity)
    {
        for (size_t index = 0; index < midiNoteEventCount; index++)
        {
            const MidiNoteEvent &event = midiNoteEvents[(midiNoteEventHead + index) % MIDI_NOTE_EVENT_CAPACITY];
            if (event.channel == channel && event.note == note &&
                static_cast<int16_t>(event.sequence - sequence) > 0)
            {
                sequence = event.sequence;
                velocity = static_cast<uint16_t>(event.velocity) << 9;
                return true;
            }
        }
        return false;
    }
    bool peek_midi_note_event(uint8_t channel, uint8_t note, uint16_t &sequence, uint16_t &velocity) const
    {
        for (size_t index = midiNoteEventCount; index > 0; --index)
        {
            const MidiNoteEvent &event = midiNoteEvents[(midiNoteEventHead + index - 1) % MIDI_NOTE_EVENT_CAPACITY];
            if (event.channel == channel && event.note == note &&
                static_cast<int16_t>(event.sequence - sequence) > 0)
            {
                sequence = event.sequence;
                velocity = static_cast<uint16_t>(event.velocity) << 9;
                return true;
            }
        }
        return false;
    }
    uint16_t read_midi_control_change(uint8_t channel, uint8_t cc) const
    {
        if (channel >= 16 || !midiControlChanges[channel] || cc >= 128)
        {
            return 0;
        }
        return midiControlChanges[channel][cc] << 9;
    }
    uint8_t read_midi_note(uint8_t channel, uint8_t note) const
    {
        if (channel >= 16 || note >= 128 || !midiNoteVelocity[channel])
        {
            return 0;
        }
        return midiNoteVelocity[channel][note];
    }
    uint16_t read_midi_pitch_bend(uint8_t channel) const
    {
        return midiPitchWheel[channel];
    }

    // Channels 0 - 15, then the Pro Guitar pseudo channels
    bool has_midi_channel(uint8_t channel) const { return seenChannels[channel]; }
    // Returns false if the channel had already been seen
    bool set_channel_seen(uint8_t channel)
    {
        if (seenChannels[channel])
        {
            return false;
        }
        seenChannels[channel] = true;
        return true;
    }
    void load_seen_channels(const bool (&seen)[18]) { memcpy(seenChannels, seen, sizeof(seenChannels)); }
    void save_seen_channels(bool (&seen)[18]) const { memcpy(seen, seenChannels, sizeof(seenChannels)); }

    // Pro Guitar state, from its sysex messages. Frets and velocities are indexed by string number - 1.
    const uint8_t (&pro_guitar_frets() const)[6] { return midiFrets; }
    const uint8_t (&pro_guitar_string_velocities() const)[6] { return midiStringVelocities; }
    const ProGuitar_Sysex_Buttons_t &pro_guitar_buttons() const { return midiButtons; }

    // The sysex numbers strings the way guitarists do, so string 1 (open E4) is high E, the reverse of
    // the low E first order the axes and the console reports use. See
    // https://docs.google.com/spreadsheets/d/1Y3QM1tEcf0bGiUTjT7R-3mwEAKrCL0qYoySmk3RLo8c (sysex format)
    // and https://github.com/jessecrossen/vst-stangin (string numbering and open notes)
    uint16_t pro_guitar_axis(ProGuitarAxisType axis) const
    {
        // we want fret inputs to stay at the standard range
        // but then everything else needs to be scaled up
        switch (axis)
        {
        case ProGuitar_LowEFret:
            return midiFrets[5];
        case ProGuitar_AFret:
            return midiFrets[4];
        case ProGuitar_DFret:
            return midiFrets[3];
        case ProGuitar_GFret:
            return midiFrets[2];
        case ProGuitar_BFret:
            return midiFrets[1];
        case ProGuitar_HighEFret:
            return midiFrets[0];
        case ProGuitar_LowEFretVelocity:
            return midiStringVelocities[5] << 8;
        case ProGuitar_AFretVelocity:
            return midiStringVelocities[4] << 8;
        case ProGuitar_DFretVelocity:
            return midiStringVelocities[3] << 8;
        case ProGuitar_GFretVelocity:
            return midiStringVelocities[2] << 8;
        case ProGuitar_BFretVelocity:
            return midiStringVelocities[1] << 8;
        case ProGuitar_HighEFretVelocity:
            return midiStringVelocities[0] << 8;
        case ProGuitar_Tilt:
            return midiButtons.tilt ? 65535 : 32767;
        case ProGuitar_AutoCalibrationMicrophone:
            return 0;
        case ProGuitar_AutoCalibrationLight:
            return 0;
        }
        return 0;
    }

    static constexpr size_t MIDI_NOTE_EVENT_CAPACITY = 32;

private:
    struct MidiNoteEvent
    {
        uint16_t sequence;
        uint8_t channel;
        uint8_t note;
        uint8_t velocity;
    };
    MidiNoteEvent midiNoteEvents[MIDI_NOTE_EVENT_CAPACITY];
    uint8_t midiNoteEventHead = 0;
    uint8_t midiNoteEventCount = 0;
    uint16_t midiNoteEventSequence = 0;
    uint16_t midiPitchWheel[16];
    uint8_t *midiControlChanges[16] = {};
    uint8_t *midiNoteVelocity[16] = {};
    uint8_t midiFrets[6];
    uint8_t midiStringVelocities[6];
    bool seenChannels[18];
    ProGuitar_Sysex_Buttons_t midiButtons;
    bool usbPackets;
    cable_state_t *cable_status = nullptr;
    uint8_t m_max_cables = 1;

    void push_midi_note_event(uint8_t channel, uint8_t note, uint8_t velocity)
    {
        MidiNoteEvent &event = midiNoteEvents[(midiNoteEventHead + midiNoteEventCount) % MIDI_NOTE_EVENT_CAPACITY];
        event.sequence = ++midiNoteEventSequence;
        event.channel = channel;
        event.note = note;
        event.velocity = velocity;

        if (midiNoteEventCount < MIDI_NOTE_EVENT_CAPACITY)
        {
            midiNoteEventCount++;
        }
        else
        {
            midiNoteEventHead = (midiNoteEventHead + 1) % MIDI_NOTE_EVENT_CAPACITY;
        }
    }

    // Number of MIDI bytes in a USB MIDI event packet with this Code Index Number (USB MIDI 1.0, table 4-1)
    static uint8_t usb_midi_cin_size(uint8_t cin)
    {
        switch (cin)
        {
        case MIDI_CIN_SYSEX_END_1BYTE:
        case MIDI_CIN_1BYTE_DATA:
            return 1;
        case MIDI_CIN_SYSCOM_2BYTE:
        case MIDI_CIN_SYSEX_END_2BYTE:
        case MIDI_CIN_PROGRAM_CHANGE:
        case MIDI_CIN_CHANNEL_PRESSURE:
            return 2;
        case MIDI_CIN_SYSCOM_3BYTE:
        case MIDI_CIN_SYSEX_START:
        case MIDI_CIN_SYSEX_END_3BYTE:
        case MIDI_CIN_NOTE_OFF:
        case MIDI_CIN_NOTE_ON:
        case MIDI_CIN_POLY_KEYPRESS:
        case MIDI_CIN_CONTROL_CHANGE:
        case MIDI_CIN_PITCH_BEND_CHANGE:
            return 3;
        default:
            // CIN 0 and 1 are reserved
            return 0;
        }
    }

    // Size of the message a status byte starts, or 0 for sysex and the undefined / stray system common
    // bytes (F4, F5 and F7 outside of a sysex) that are ignored
    static uint8_t midi_message_size(uint8_t status)
    {
        if (status < MIDI_STATUS_SYSEX_START)
        {
            switch (status >> 4)
            {
            case MIDI_CIN_PROGRAM_CHANGE:
            case MIDI_CIN_CHANNEL_PRESSURE:
                return 2;
            default:
                return 3;
            }
        }
        switch (status)
        {
        case MIDI_STATUS_SYSCOM_TIME_CODE_QUARTER_FRAME:
        case MIDI_STATUS_SYSCOM_SONG_SELECT:
            return 2;
        case MIDI_STATUS_SYSCOM_SONG_POSITION_POINTER:
            return 3;
        case MIDI_STATUS_SYSCOM_TUNE_REQUEST:
            return 1;
        default:
            return 0;
        }
    }

    template <typename OnMessage, typename MarkChannelSeen>
    void complete_message(const uint8_t *data, unsigned int size, OnMessage &on_message, MarkChannelSeen &mark_channel_seen)
    {
        on_message(data, (uint8_t)size);
        process_message(data, mark_channel_seen);
    }

    // Parse one byte of a MIDI 1.0 byte stream
    template <typename OnMessage, typename MarkChannelSeen>
    void parse_byte(cable_state_t *cable_state, uint8_t byte, OnMessage &on_message, MarkChannelSeen &mark_channel_seen)
    {
        if (byte >= MIDI_STATUS_SYSREAL_TIMING_CLOCK)
        {
            // System Realtime messages are single bytes that can go anywhere, even in the middle of
            // another message, and don't affect running status
            uint8_t realtime[3] = {byte, 0, 0};
            complete_message(realtime, 1, on_message, mark_channel_seen);
            return;
        }
        if (byte <= MIDI_MAX_DATA_VAL)
        {
            if (cable_state->sysex_in_progress)
            {
                // keep counting past the end of the buffer, so a message too long to keep can be dropped
                if (cable_state->pos < sizeof(cable_state->data))
                {
                    cable_state->data[cable_state->pos] = byte;
                }
                cable_state->pos++;
                return;
            }
            if (cable_state->pos == 0)
            {
                if (!cable_state->status)
                {
                    // no running status, so this doesn't belong to anything
                    return;
                }
                // Running status, the status byte is reused
                cable_state->data[0] = cable_state->status;
                cable_state->pos = 1;
            }
            cable_state->data[cable_state->pos++] = byte;
            if (cable_state->pos >= cable_state->actual_size)
            {
                cable_state->pos = 0;
                complete_message(cable_state->data, cable_state->actual_size, on_message, mark_channel_seen);
            }
            return;
        }
        // Any other status byte ends a sysex message, F7 is optional
        if (cable_state->sysex_in_progress)
        {
            unsigned int size = cable_state->pos;
            cable_state->sysex_in_progress = false;
            cable_state->pos = 0;
            if (byte == MIDI_STATUS_SYSEX_END)
            {
                if (size < sizeof(cable_state->data))
                {
                    cable_state->data[size] = byte;
                }
                size++;
            }
            // drop messages that didn't fit in the buffer
            if (size <= sizeof(cable_state->data))
            {
                complete_message(cable_state->data, size, on_message, mark_channel_seen);
            }
            if (byte == MIDI_STATUS_SYSEX_END)
            {
                return;
            }
        }
        // A new message, abandoning any incomplete one
        cable_state->data[0] = byte;
        cable_state->pos = 1;
        cable_state->actual_size = midi_message_size(byte);
        // Running status only applies to channel voice messages, sysex and system common messages cancel it
        cable_state->status = byte < MIDI_STATUS_SYSEX_START ? byte : 0;
        if (byte == MIDI_STATUS_SYSEX_START)
        {
            cable_state->sysex_in_progress = true;
        }
        else if (cable_state->actual_size <= 1)
        {
            cable_state->pos = 0;
            if (cable_state->actual_size)
            {
                complete_message(cable_state->data, 1, on_message, mark_channel_seen);
            }
        }
    }

    // Apply a complete message
    template <typename MarkChannelSeen>
    void process_message(const uint8_t *data, MarkChannelSeen &mark_channel_seen)
    {
        uint8_t status = (data[0] & 0xf0) >> 4;
        uint8_t channel = data[0] & 0x0f;
        switch (status)
        {
        case MIDI_CIN_NOTE_OFF:
            if (midiNoteVelocity[channel])
            {
                midiNoteVelocity[channel][data[1]] = 0;
            }
            break;
        case MIDI_CIN_NOTE_ON:
            if (data[2] != 0)
            {
                if (!midiNoteVelocity[channel])
                {
                    midiNoteVelocity[channel] = new uint8_t[128]();
                }
                midiNoteVelocity[channel][data[1]] = data[2];
                push_midi_note_event(channel, data[1], data[2]);
            }
            else
            {
                if (midiNoteVelocity[channel])
                {
                    midiNoteVelocity[channel][data[1]] = 0;
                }
            }
            break;
        case MIDI_CIN_CONTROL_CHANGE:
            if (!midiControlChanges[channel])
            {
                midiControlChanges[channel] = new uint8_t[128]();
            }
            midiControlChanges[channel][data[1]] = data[2];
            break;
        case MIDI_CIN_PITCH_BEND_CHANGE:
            midiPitchWheel[channel] = ((uint16_t)data[2] << 7) | data[1];
            break;
        case MIDI_CIN_POLY_KEYPRESS:
            break;
        case MIDI_CIN_PROGRAM_CHANGE:
        case MIDI_CIN_CHANNEL_PRESSURE:
            break;
        default:
            break;
        }
        if (data[0] < MIDI_STATUS_SYSEX_START)
        {
            mark_channel_seen(channel);
        }
        if (data[0] == MIDI_STATUS_SYSEX_START)
        {
            uint8_t buttons_header[] = {MIDI_STATUS_SYSEX_START, 0x08, 0x40};
            if (memcmp(data, buttons_header, sizeof(buttons_header)) == 0)
            {
                if (data[3] == MIDI_SYSEX_ID_PROGUITAR_SQUIER && !seenChannels[MIDI_CHANNEL_PROGUITAR_SQUIER])
                {
                    mark_channel_seen(MIDI_CHANNEL_PROGUITAR_SQUIER);
                }
                if (data[3] == MIDI_SYSEX_ID_PROGUITAR_MUSTANG && !seenChannels[MIDI_CHANNEL_PROGUITAR_MUSTANG])
                {
                    mark_channel_seen(MIDI_CHANNEL_PROGUITAR_MUSTANG);
                }
                if (data[4] == 0x08)
                {
                    // button events
                    memcpy(&midiButtons, data, sizeof(midiButtons));
                }
                if (data[4] == 0x01)
                {
                    // fret state events
                    uint8_t string = data[5] - 1;
                    switch (string)
                    {
                    case 0:
                        midiFrets[string] = data[6] - 0x40;
                        break;
                    case 1:
                        midiFrets[string] = data[6] - 0x3B;
                        break;
                    case 2:
                        midiFrets[string] = data[6] - 0x37;
                        break;
                    case 3:
                        midiFrets[string] = data[6] - 0x32;
                        break;
                    case 4:
                        midiFrets[string] = data[6] - 0x2D;
                        break;
                    case 5:
                        midiFrets[string] = data[6] - 0x28;
                        break;
                    default:
                        break;
                    }
                }
                // like frets, only strings 1 - 6 exist
                if (data[4] == 0x05 && data[5] >= 1 && data[5] <= TU_ARRAY_SIZE(midiStringVelocities))
                {
                    // picking events
                    uint8_t string = data[5] - 1;
                    uint8_t velocity = data[6];
                    // picking the same velocity twice flips the lowest bit so it still shows up as a change
                    if (midiStringVelocities[string] == velocity)
                    {
                        velocity ^= 1;
                    }
                    midiStringVelocities[string] = velocity;
                }
            }
        }
    }
};
