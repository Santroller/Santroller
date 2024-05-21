#pragma once

#include <cstdint>
#include <MIDI.h>
#include "TUSB-MIDI_defs.hpp"
#include "tusb.h"

namespace usbMidi {

    class UsbMidiTransport {
    private:
        byte mTxBuffer[4];
        size_t mTxIndex;
        MidiType mTxStatus;

        byte mRxBuffer[4];
        size_t mRxLength;
        size_t mRxIndex;

        midiEventPacket_t mPacket;
        uint8_t cableNumber;

        static const size_t MIDI_QUEUE_SIZE = 16;
        midiEventPacket_t midiQueue[MIDI_QUEUE_SIZE] = {{0}};
        size_t midiQueueIndex = 0;

    public:
        UsbMidiTransport(uint8_t cableNumber = 0) {
            this->cableNumber = cableNumber;
        };

    public:
        uint8_t midi_dev_addr = 0;

        static const bool thruActivated = false;

        void begin() {
            mTxIndex = 0;
            mRxIndex = 0;
            mRxLength = 0;
        };

        void pollUsb() {
            if (!midi_dev_addr || !tuh_midi_configured(midi_dev_addr)) {
                return;
            }
            if (tuh_midih_get_num_rx_cables(midi_dev_addr) < 1) {
                return;
            }

            tuh_midi_read_poll(midi_dev_addr); // if there is data, then the callback will be called
            tuh_midi_stream_flush(midi_dev_addr);
        }

        void tuh_midi_rx_cb(uint8_t dev_addr, uint32_t num_packets) {
            if (midi_dev_addr != dev_addr) return;

            while (num_packets > 0) {
                --num_packets;
                uint8_t bytes[4];
                if (tuh_midi_packet_read(dev_addr, bytes)) {
                    TU_LOG1("Read bytes %u %u %u %u\r\n", bytes[0], bytes[1], bytes[2], bytes[3]);

                    midiEventPacket_t packet = {
                            .header = bytes[0],
                            .byte1 = bytes[1],
                            .byte2 = bytes[2],
                            .byte3 = bytes[3]
                    };

                    if(midiQueueIndex < MIDI_QUEUE_SIZE - 1) {
                        midiQueue[midiQueueIndex] = packet;
                        midiQueueIndex++;
                    } else {
                        TU_LOG1("Buffer overflow");
                    }
                }
            }
        }

        void end() {
        }

        bool beginTransmission(MidiType status) {
            mTxStatus = status;

            byte cin = 0;
            if (status < SystemExclusive) {
                // Non System messages
                cin = type2cin[((status & 0xF0) >> 4) - 7][1];
                mPacket.header = MAKEHEADER(cableNumber, cin);
            } else {
                // Only System messages
                cin = system2cin[status & 0x0F][1];
                mPacket.header = MAKEHEADER(cableNumber, cin);
            }

            mPacket.byte1 = mPacket.byte2 = mPacket.byte3 = 0;
            mTxIndex = 0;

            return true;
        };

        void write(byte byte) {
            if (mTxStatus != MidiType::SystemExclusive) {
                if (mTxIndex == 0) mPacket.byte1 = byte;
                else if (mTxIndex == 1) mPacket.byte2 = byte;
                else if (mTxIndex == 2) mPacket.byte3 = byte;
            } else if (byte == MidiType::SystemExclusiveStart) {
                mPacket.header = MAKEHEADER(cableNumber, 0x04);
                mPacket.byte1 = byte;
            } else // SystemExclusiveEnd or SysEx data
            {
                auto i = mTxIndex % 3;
                if (byte == MidiType::SystemExclusiveEnd)
                    mPacket.header = MAKEHEADER(cableNumber, (0x05 + i));

                if (i == 0) {
                    mPacket.byte1 = byte;
                    mPacket.byte2 = mPacket.byte3 = 0x00;
                } else if (i == 1) {
                    mPacket.byte2 = byte;
                    mPacket.byte3 = 0x00;
                } else if (i == 2) {
                    mPacket.byte3 = byte;
                    if (byte != MidiType::SystemExclusiveEnd) {
                        tuh_midi_packet_write(midi_dev_addr, (uint8_t*)&mPacket);
                    }
                }
            }
            mTxIndex++;
        };

        void endTransmission() {
            tuh_midi_packet_write(midi_dev_addr, (uint8_t*)&mPacket);
        };

        byte read() {
            RXBUFFER_POPFRONT(byte);
            return byte;
        };

        unsigned available() {
            // consume mRxBuffer first, before getting a new packet
            if (mRxLength > 0)
                return mRxLength;

            mRxIndex = 0;
            if(midiQueueIndex > 0) {
                mPacket = midiQueue[midiQueueIndex - 1];
                midiQueueIndex--;
            } else {
                pollUsb();
                mPacket.header = 0;
            }

            if (mPacket.header != 0) {
                auto cn = GETCABLENUMBER(mPacket);
                if (cn != cableNumber)
                    return 0;

                auto cin = GETCIN(mPacket);
                auto len = cin2Len[cin][1];
                switch (len) {
                    case 0:
                        if (cin == 0x4 || cin == 0x7) RXBUFFER_PUSHBACK3
                        else if (cin == 0x5) RXBUFFER_PUSHBACK1
                        else if (cin == 0x6) RXBUFFER_PUSHBACK2
                        break;
                    case 1: RXBUFFER_PUSHBACK1
                        break;
                    case 2: RXBUFFER_PUSHBACK2
                        break;
                    case 3: RXBUFFER_PUSHBACK3
                        break;
                    default:
                        break; // error
                }
            }

            return mRxLength;
        };
    };

}

/*! \brief
 */
#define USBMIDI_CREATE_INSTANCE(CableNr, Name)  \
    USBMIDI_NAMESPACE::usbMidiTransport __usb##Name(CableNr);\
    MIDI_NAMESPACE::MidiInterface<USBMIDI_NAMESPACE::usbMidiTransport> Name((USBMIDI_NAMESPACE::usbMidiTransport&)__usb##Name);

#define USBMIDI_CREATE_CUSTOM_INSTANCE(CableNr, Name, Settings)  \
    USBMIDI_NAMESPACE::usbMidiTransport __usb##Name(CableNr);\
    MIDI_NAMESPACE::MidiInterface<USBMIDI_NAMESPACE::usbMidiTransport, Settings> Name((USBMIDI_NAMESPACE::usbMidiTransport&)__usb##Name);

#define USBMIDI_CREATE_DEFAULT_INSTANCE()  \
    USBMIDI_CREATE_INSTANCE(0, MIDI)
