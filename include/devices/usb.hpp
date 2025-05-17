#pragma once
#include <MIDI.h>

#include <SimpleMidiTransport.hpp>

#include "i2c.hpp"
#include "parsers/base.hpp"
#include "state/base.hpp"
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
   public:
    USBDevice(Parser& parser) : mParser(parser) {};
    void tick(san_base_t* data);

   private:
    MIDI_NAMESPACE::SimpleMidiInterface midiInterface;
    uint8_t mReportData[128];
    Parser& mParser;
};