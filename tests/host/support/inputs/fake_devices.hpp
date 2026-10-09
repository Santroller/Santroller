#pragma once
// State behind the DeviceManager functions faked in fakes.cpp
namespace fake_devices
{
// Whether a console is talking to the Wii extension / PS2 controller port emulation
inline bool wii_communicating = false;
inline bool psx_communicating = false;
inline void reset()
{
    wii_communicating = false;
    psx_communicating = false;
}
}
