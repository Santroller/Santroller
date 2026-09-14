#pragma once
#include <stdint.h>
#include "enums.pb.h"
#include "input_enums.pb.h"
#include "input.pb.h"

class MidiDevice;

class WiiExtensionDecoder
{
public:
    WiiExtType mType = WiiExtType::WiiNoExtension;
    uint8_t mBuffer[8] = {};
    bool hiRes = false;
    bool hasTapBar = false;
    uint8_t s_box = 0;

    void reset();

    // Decodes 6-byte extension ID read from register 0xFA / 0xa400fa
    void decode_id(const uint8_t *id_bytes);

    // Updates buffer with extension bytes; handles s_box decryption,
    // tap bar detection, and GH drum MIDI parsing
    void update_data(const uint8_t *data, uint8_t len, MidiDevice *midi_device = nullptr);

    // Wii direct input reading
    uint16_t read_axis(proto_WiiAxisType type) const;
    bool read_button(proto_WiiButtonType type) const;

    // Output mapping for assignable host interfaces (BT / USB)
    bool tick_digital(proto_Output &type) const;
    uint16_t tick_analog(proto_Output &type) const;

    // SubType mapping helper
    SubType get_subtype() const;
};

