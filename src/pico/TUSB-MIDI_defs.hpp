#pragma once

#include <cstdint>

namespace usbMidi {

    using namespace MIDI_NAMESPACE;

// from https://www.usb.org/sites/default/files/midi10.pdf
// 4 USB-MIDI Event Packets
// Table 4-1: Code Index Number Classifications

    static uint8_t type2cin[][2] = { {MidiType::InvalidType,0}, {MidiType::NoteOff,8}, {MidiType::NoteOn,9}, {MidiType::AfterTouchPoly,0xA}, {MidiType::ControlChange,0xB}, {MidiType::ProgramChange,0xC}, {MidiType::AfterTouchChannel,0xD}, {MidiType::PitchBend,0xE} };

    static uint8_t system2cin[][2] = { {MidiType::SystemExclusive,0}, {MidiType::TimeCodeQuarterFrame,2}, {MidiType::SongPosition,3}, {MidiType::SongSelect,2}, {0,0}, {0,0}, {MidiType::TuneRequest,5}, {MidiType::SystemExclusiveEnd,0}, {MidiType::Clock,0xF}, {0,0}, {MidiType::Start,0xF}, {MidiType::Continue,0xF}, {MidiType::Stop,0xF}, {0,0}, {MidiType::ActiveSensing,0xF}, {MidiType::SystemReset,0xF} };

    static uint8_t cin2Len[][2] = { {0,0}, {1,0}, {2,2}, {3,3}, {4,0}, {5,0}, {6,0}, {7,0}, {8,3}, {9,3}, {10,3}, {11,3}, {12,2}, {13,2}, {14,3}, {15,1} };

#define GETCABLENUMBER(packet) (packet.header >> 4);
#define GETCIN(packet) (packet.header & 0x0f);
#define MAKEHEADER(cn, cin) (((cn & 0x0f) << 4) | cin)
#define RXBUFFER_PUSHBACK1 { mRxBuffer[mRxLength++] = mPacket.byte1; }
#define RXBUFFER_PUSHBACK2 { mRxBuffer[mRxLength++] = mPacket.byte1; mRxBuffer[mRxLength++] = mPacket.byte2; }
#define RXBUFFER_PUSHBACK3 { mRxBuffer[mRxLength++] = mPacket.byte1; mRxBuffer[mRxLength++] = mPacket.byte2; mRxBuffer[mRxLength++] = mPacket.byte3; }
#define RXBUFFER_POPFRONT(byte) auto byte = mRxBuffer[mRxIndex++]; mRxLength--;
//#define SENDMIDI(packet) { MidiUSB.sendMIDI(packet); MidiUSB.flush(); }

}

typedef struct
{
    uint8_t header;
    uint8_t byte1;
    uint8_t byte2;
    uint8_t byte3;
} midiEventPacket_t;