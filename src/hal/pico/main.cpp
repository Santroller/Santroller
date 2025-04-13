#include "Arduino.h"
#include "hal/pico/interfaces/i2c.hpp"
#include "interfaces/i2c.hpp"
#include "devices/bh_drum.hpp"
#include "devices/midi.hpp"
void loop() {

}
PicoI2CMasterInterface i2cInterface;
BHMidiTransport transport(&i2cInterface);
MIDIDevice<BHMidiTransport> bhMidi(transport);
void setup() {
    bhMidi.begin();
}