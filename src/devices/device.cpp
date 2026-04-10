#include "devices/base.hpp"
bool Device::is_wii_extension(WiiExtType type)
{
    return false;
}
bool Device::is_usb_device(proto_SpecificUsbDevice type)
{
    return false;
}
bool Device::is_usb_type(SubType type)
{
    return false;
}
bool Device::is_bluetooth_device(proto_SpecificUsbDevice type)
{
    return false;
}
bool Device::is_bluetooth_type(SubType type)
{
    return false;
}
bool Device::is_ps2_device(PS2ControllerType type)
{
    return false;
}
bool Device::has_midi_channel(uint8_t channel)
{
    return false;
}
void Device::rescan(bool first) {}