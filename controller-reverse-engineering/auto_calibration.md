# Rock Band Auto-Calibration Sensors

## Xbox 360
The sensors can be activated by sending the following right motor values:

```cpp
#include <Xinput.h>

#define XBOX360_AUTOCAL_MICROPHONE 0x6000
#define XBOX360_AUTOCAL_LIGHT 0xFFFF
#define XBOX360_AUTOCAL_DISABLE 0x0000

void enableMic(int userIndex)
{
    XINPUT_VIBRATION vibration = { 0, XBOX360_AUTOCAL_MICROPHONE };
    XInputSetState(userIndex, &vibration);
}

void enableLight(int userIndex)
{
    XINPUT_VIBRATION vibration = { 0, XBOX360_AUTOCAL_LIGHT };
    XInputSetState(userIndex, &vibration);
}

void disableSensors(int userIndex)
{
    XINPUT_VIBRATION vibration = { 0, XBOX360_AUTOCAL_DISABLE };
    XInputSetState(userIndex, &vibration);
}
```

## Wii
Activating the sensors can be done by sending the following HID feature report sequences:

```csharp
using HidSharp;

byte[] activate_mic = {0x00, 0xE9, 0x00, 0x83, 0x1B, 0x00, 0x00, 0x00, 0x02};
byte[] activate_light = {0x00, 0xE9, 0x00, 0x83, 0x1B, 0x00, 0x00, 0x00, 0x01};
byte[] disable_mic_light = {0x00, 0xE9, 0x00, 0x83, 0x1B, 0x00, 0x00, 0x00, 0x00};
byte[][] other_commands = new byte[4][] {
   new byte[9] {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
   new byte[9] {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00},
   new byte[9] {0x00, 0x00, 0x00, 0x83, 0x00, 0x00, 0x00, 0x00, 0x00},
   new byte[9] {0x00, 0xE9, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
};

private bool sendPacket(HidDevice device, byte[] packet)
{
    var stream = device.Open();
    stream.SetFeature(packet);

    foreach (byte[] cmd in other_commands) {
        stream.SetFeature(cmd);
    }

    // Optional: get feature report to get the device's response
    // The report will be all 0s if the sensor was successfully enabled,
    // and will be different once deactivated
    byte[] data = new byte[0x1C];
    stream.GetFeature(data);
    stream.Close();

    return true;
}

public void enableMic(HidDevice device)
{
    sendPacket(device, activate_mic);
}
public void enableLight(HidDevice device)
{
    sendPacket(device, activate_light);
}
public void disableSensors(HidDevice device)
{
    sendPacket(device, disable_mic_light);
}
```

The audio data can be read from byte 15 of the standard input report. With no noise the value is `0x7F`, and when a chirp is detected, the value will lower, with a minimum of `0x3F`.

The light sensor data can be read from byte 16 of the standard input report. The value will be `0x00` in darkness, and will read up to `0x7F` when a flash is detected.

Reference: https://github.com/dynamix1337/AutoCalibrationRB

# Ps3
Activating the sensors is as simple as sending the following as HID OUTPUT Reports, and then reading the left joystick x axis (first axis):

```csharp
byte[] activate_mic_ps3 = {0x00, 0x01, 0x08, 0x01, 0x40, 0x00, 0x00, 0x00, 0x00};
byte[] activate_light_ps3 = {0x00, 0x01, 0x08, 0x01, 0xFF, 0x00, 0x00, 0x00, 0x00};
byte[] disable_mic_light_ps3 = {0x00, 0x01, 0x08, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00};

public void enableMicPs3(HidDevice device)
{
    var stream = device.Open();
    stream.Write(activate_mic_ps3);
    stream.Close();
}

public void enableLightPs3(HidDevice device)
{
    var stream = device.Open();
    stream.Write(activate_light_ps3);
    stream.Close();
}

public void disableSensorsPs3(HidDevice device)
{
    var stream = device.Open();
    stream.Write(disable_mic_light_ps3);
    stream.Close();
}
```