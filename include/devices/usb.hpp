#pragma once
#include <MIDI.h>

#include <SimpleMidiTransport.hpp>

#include "i2c.hpp"
#include "base.hpp"
#include "device.pb.h"
#include "libmpr121.hpp"
#pragma once
enum USBDeviceType
{
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
// Only for non midi, since we use the standard midi lib here for midi usb already
// Drum kits (and pro keys) pipe to midi, gh kits pipe to the midi library
class USBDevice : public Device
{
public:
    ~USBDevice() {}
    USBDevice(proto_UsbHostDevice device, uint16_t id);
    void update(bool full_poll);

private:
    proto_UsbHostDevice m_device;
    MIDI_NAMESPACE::SimpleMidiInterface midiInterface;
    uint8_t mReportData[128];
    // Parser &mParser;
};