#pragma once
#include <MIDI.h>

#include "devices/midi.hpp"
#include "interfaces/i2c.hpp"
#include "state/base.hpp"
#include "parsers/base.hpp"
using namespace MIDI_NAMESPACE;
#pragma once
enum USBDeviceType {
    Santroller,
    XInput,
    PS3,
    PS4,
    PS5,
    Raphnet,
    Switch,
    XboxOne,
    OGXbox
};
enum USBControllerType {
    Gamepad,
    Dancepad,
    GuitarHeroGuitar,
    GuitarHeroGuitarWt,
    RockBandGuitar,
    GuitarHeroDrums,
    RockBandDrums,
    LiveGuitar,
    DjHeroTurntable,
    ProGuitarMustang,
    ProGuitarSquire,
    ProKeys,
    Taiko,
    StageKit,
    Keyboard,
    Mouse,
    Wheel,
    DisneyInfinity,
    Skylanders,
    LegoDimensions,
};
// Only for non midi, since we use the standard midi lib here for midi usb already
// Drum kits (and pro keys) pipe to midi, gh kits pipe to the midi library
class USBDevice {
    friend class MIDI_NAMESPACE::MidiInterface<USBDevice>;

   public:
    USBDevice(Parser& parser):mParser(parser) {};

    inline void begin() {
    }

    inline void end() {
    }
    void tick(san_base_t* data);

   protected:
    static const bool thruActivated = false;
    // Read only, we don't ever send
    inline bool beginTransmission(MidiType) {
        return false;
    };

    inline void write(byte byte) {

    };

    inline void endTransmission() {
    };

    inline byte read() {
        return mBuffer[mBufferIndex];
    };

    unsigned available();

   private:
    uint8_t mBufferIndex;
    uint8_t mBuffer[6];
    uint8_t mReportData[128];
    Parser& mParser;
};