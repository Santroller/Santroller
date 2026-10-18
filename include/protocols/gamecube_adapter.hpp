#pragma once
#include <stdint.h>
#include <string.h>
#include "input.pb.h"

// ---------------------------------------------------------------------------
// USB GameCube controller adapters, following SDL's HIDAPI GameCube driver
//
// Nintendo's Wii U adapter (WUP-028) runs all four ports through one interface. It stays quiet until it
// is sent 0x13, then sends report 0x21: a status byte and 8 bytes of controller data for each port. Its
// rumble report is 0x11 and an on / off byte per port.
//
// EVORETRO's adapters in PC mode are plain HID gamepads. Older firmware sends a 10 byte report for every
// port, whose first byte is the port (1 to 4). Firmware 0x7 and newer has an interface per port and sends 9
// bytes with no port, and only that firmware takes rumble.
// ---------------------------------------------------------------------------

#define GAMECUBE_ADAPTER_PORTS 4

#define DRAGONRISE_VID 0x0079
#define NINTENDO_GAMECUBE_ADAPTER_PID 0x0337
#define EVORETRO_GAMECUBE_ADAPTER_PID_1 0x1843
#define EVORETRO_GAMECUBE_ADAPTER_PID_2 0x1844
#define EVORETRO_GAMECUBE_ADAPTER_PID_3 0x1846

#define GAMECUBE_ADAPTER_INIT 0x13
#define GAMECUBE_ADAPTER_INPUT_REPORT_ID 0x21
#define GAMECUBE_ADAPTER_INPUT_REPORT_LEN 37
#define GAMECUBE_ADAPTER_RUMBLE_REPORT_ID 0x11
#define GAMECUBE_ADAPTER_RUMBLE_REPORT_LEN 5
#define EVORETRO_REPORT_LEN 9
#define EVORETRO_REPORT_WITH_PORT_LEN 10
#define EVORETRO_RUMBLE_REPORT_LEN 2
// EVORETRO adapters stop rumbling unless the rumble report keeps coming
#define EVORETRO_RUMBLE_REFRESH_MS 16

// Nintendo port status byte
#define GAMECUBE_ADAPTER_STATUS_WIRED 0x10
#define GAMECUBE_ADAPTER_STATUS_WIRELESS 0x20
// the adapter's second (power) cable is plugged in
#define GAMECUBE_ADAPTER_STATUS_RUMBLE_POWER 0x04

enum GameCubeButton : uint16_t
{
    GAMECUBE_BUTTON_A = 1 << 0,
    GAMECUBE_BUTTON_B = 1 << 1,
    GAMECUBE_BUTTON_X = 1 << 2,
    GAMECUBE_BUTTON_Y = 1 << 3,
    GAMECUBE_BUTTON_START = 1 << 4,
    GAMECUBE_BUTTON_Z = 1 << 5,
    // the clicks at the bottom of the analog triggers
    GAMECUBE_BUTTON_L = 1 << 6,
    GAMECUBE_BUTTON_R = 1 << 7,
    GAMECUBE_BUTTON_DPAD_UP = 1 << 8,
    GAMECUBE_BUTTON_DPAD_DOWN = 1 << 9,
    GAMECUBE_BUTTON_DPAD_LEFT = 1 << 10,
    GAMECUBE_BUTTON_DPAD_RIGHT = 1 << 11,
};

enum GameCubeAxis : uint8_t
{
    GAMECUBE_AXIS_STICK_X,
    GAMECUBE_AXIS_STICK_Y,
    GAMECUBE_AXIS_C_STICK_X,
    GAMECUBE_AXIS_C_STICK_Y,
    GAMECUBE_AXIS_L,
    GAMECUBE_AXIS_R,
    GAMECUBE_AXIS_COUNT
};

// One port, in the controller's own terms: axes as the controller reports them, up and right high
struct GameCubePortState
{
    bool connected = false;
    // only the Nintendo adapter reports these
    bool wireless = false;
    bool rumble_power = false;
    uint16_t buttons = 0;
    uint8_t axes[GAMECUBE_AXIS_COUNT] = {128, 128, 128, 128, 0, 0};
};

static inline void gamecube_set(uint16_t &buttons, bool pressed, uint16_t button)
{
    if (pressed)
    {
        buttons |= button;
    }
}

// Report 0x21 into all four ports. False if it isn't one
static inline bool gamecube_adapter_parse(const uint8_t *buf, uint32_t len, GameCubePortState ports[GAMECUBE_ADAPTER_PORTS])
{
    if (len < GAMECUBE_ADAPTER_INPUT_REPORT_LEN || buf[0] != GAMECUBE_ADAPTER_INPUT_REPORT_ID)
    {
        return false;
    }
    for (uint8_t i = 0; i < GAMECUBE_ADAPTER_PORTS; i++)
    {
        const uint8_t *slot = buf + 1 + i * 9;
        auto &port = ports[i];
        port.connected = slot[0] & (GAMECUBE_ADAPTER_STATUS_WIRED | GAMECUBE_ADAPTER_STATUS_WIRELESS);
        port.wireless = slot[0] & GAMECUBE_ADAPTER_STATUS_WIRELESS;
        port.rumble_power = slot[0] & GAMECUBE_ADAPTER_STATUS_RUMBLE_POWER;
        uint16_t buttons = 0;
        gamecube_set(buttons, slot[1] & 0x01, GAMECUBE_BUTTON_A);
        gamecube_set(buttons, slot[1] & 0x02, GAMECUBE_BUTTON_B);
        gamecube_set(buttons, slot[1] & 0x04, GAMECUBE_BUTTON_X);
        gamecube_set(buttons, slot[1] & 0x08, GAMECUBE_BUTTON_Y);
        gamecube_set(buttons, slot[1] & 0x10, GAMECUBE_BUTTON_DPAD_LEFT);
        gamecube_set(buttons, slot[1] & 0x20, GAMECUBE_BUTTON_DPAD_RIGHT);
        gamecube_set(buttons, slot[1] & 0x40, GAMECUBE_BUTTON_DPAD_DOWN);
        gamecube_set(buttons, slot[1] & 0x80, GAMECUBE_BUTTON_DPAD_UP);
        gamecube_set(buttons, slot[2] & 0x01, GAMECUBE_BUTTON_START);
        gamecube_set(buttons, slot[2] & 0x02, GAMECUBE_BUTTON_Z);
        gamecube_set(buttons, slot[2] & 0x04, GAMECUBE_BUTTON_R);
        gamecube_set(buttons, slot[2] & 0x08, GAMECUBE_BUTTON_L);
        port.buttons = buttons;
        memcpy(port.axes, slot + 3, GAMECUBE_AXIS_COUNT);
    }
    return true;
}

// An EVORETRO report into the port it is for. False if it isn't one. port is 0 for the 9 byte reports,
// which only ever have the one port on their interface
static inline bool evoretro_parse(const uint8_t *buf, uint32_t len, GameCubePortState ports[GAMECUBE_ADAPTER_PORTS], uint8_t &port_index)
{
    // Older firmware's C stick reads backwards compared to the main stick on X, newer firmware's on Y.
    // Like the main stick's Y, those are flipped to match the controller
    bool older = len == EVORETRO_REPORT_WITH_PORT_LEN;
    if (older)
    {
        if (buf[0] < 1 || buf[0] > GAMECUBE_ADAPTER_PORTS)
        {
            return false;
        }
        port_index = buf[0] - 1;
        buf++;
    }
    else if (len == EVORETRO_REPORT_LEN)
    {
        port_index = 0;
    }
    else
    {
        return false;
    }
    auto &port = ports[port_index];
    // nothing says whether a controller is plugged in, so a port counts once it reports
    port.connected = true;
    uint16_t buttons = 0;
    gamecube_set(buttons, buf[0] & 0x01, GAMECUBE_BUTTON_X);
    gamecube_set(buttons, buf[0] & 0x02, GAMECUBE_BUTTON_A);
    gamecube_set(buttons, buf[0] & 0x04, GAMECUBE_BUTTON_B);
    gamecube_set(buttons, buf[0] & 0x08, GAMECUBE_BUTTON_Y);
    gamecube_set(buttons, buf[0] & 0x10, GAMECUBE_BUTTON_L);
    gamecube_set(buttons, buf[0] & 0x20, GAMECUBE_BUTTON_R);
    gamecube_set(buttons, buf[0] & 0x80, GAMECUBE_BUTTON_Z);
    gamecube_set(buttons, buf[1] & 0x02, GAMECUBE_BUTTON_START);
    gamecube_set(buttons, buf[1] & 0x10, GAMECUBE_BUTTON_DPAD_UP);
    gamecube_set(buttons, buf[1] & 0x20, GAMECUBE_BUTTON_DPAD_RIGHT);
    gamecube_set(buttons, buf[1] & 0x40, GAMECUBE_BUTTON_DPAD_DOWN);
    gamecube_set(buttons, buf[1] & 0x80, GAMECUBE_BUTTON_DPAD_LEFT);
    port.buttons = buttons;
    port.axes[GAMECUBE_AXIS_STICK_X] = buf[2];
    port.axes[GAMECUBE_AXIS_STICK_Y] = 0xFF - buf[3];
    // the C stick comes Y first
    port.axes[GAMECUBE_AXIS_C_STICK_X] = older ? 0xFF - buf[5] : buf[5];
    port.axes[GAMECUBE_AXIS_C_STICK_Y] = older ? buf[4] : 0xFF - buf[4];
    port.axes[GAMECUBE_AXIS_L] = buf[6];
    port.axes[GAMECUBE_AXIS_R] = buf[7];
    return true;
}

// GameCube sticks don't reach the ends of their range, and each controller stops somewhere different. So
// like SDL, start from a range most controllers cover and widen it to whatever the controller reaches
struct GameCubeAxisRange
{
    uint8_t min[GAMECUBE_AXIS_COUNT];
    uint8_t max[GAMECUBE_AXIS_COUNT];

    GameCubeAxisRange() { reset(); }

    void reset()
    {
        memset(min, 128 - 88, sizeof(min));
        memset(max, 128 + 88, sizeof(max));
    }

    void update(const GameCubePortState &port)
    {
        for (uint8_t i = 0; i < GAMECUBE_AXIS_COUNT; i++)
        {
            if (port.axes[i] < min[i])
            {
                min[i] = port.axes[i];
            }
            if (port.axes[i] > max[i])
            {
                max[i] = port.axes[i];
            }
        }
    }

    uint16_t scale(const GameCubePortState &port, GameCubeAxis axis) const
    {
        int32_t value = port.axes[axis];
        int32_t lo = min[axis];
        int32_t hi = max[axis];
        if (value <= lo)
        {
            return 0;
        }
        if (value >= hi)
        {
            return UINT16_MAX;
        }
        return (value - lo) * UINT16_MAX / (hi - lo);
    }
};

static inline bool gamecube_tick_digital(const GameCubePortState &port, proto_Output &type)
{
    if (type.which_mapping != proto_Output_gamepadButton_tag)
    {
        return false;
    }
    switch (type.mapping.gamepadButton)
    {
    // by legend. The GameCube's face buttons aren't a rotated Xbox or Nintendo layout, so there is no
    // position based reading of them
    case Gamepad_A:             return port.buttons & GAMECUBE_BUTTON_A;
    case Gamepad_B:             return port.buttons & GAMECUBE_BUTTON_B;
    case Gamepad_X:             return port.buttons & GAMECUBE_BUTTON_X;
    case Gamepad_Y:             return port.buttons & GAMECUBE_BUTTON_Y;
    case Gamepad_Start:         return port.buttons & GAMECUBE_BUTTON_START;
    // Z sits above R, where SDL and Dolphin put it
    case Gamepad_RightShoulder: return port.buttons & GAMECUBE_BUTTON_Z;
    case Gamepad_DpadUp:        return port.buttons & GAMECUBE_BUTTON_DPAD_UP;
    case Gamepad_DpadDown:      return port.buttons & GAMECUBE_BUTTON_DPAD_DOWN;
    case Gamepad_DpadLeft:      return port.buttons & GAMECUBE_BUTTON_DPAD_LEFT;
    case Gamepad_DpadRight:     return port.buttons & GAMECUBE_BUTTON_DPAD_RIGHT;
    default:                    return false;
    }
}

static inline uint16_t gamecube_tick_analog(const GameCubePortState &port, const GameCubeAxisRange &range, proto_Output &type)
{
    if (type.which_mapping != proto_Output_gamepadAxis_tag)
    {
        return 0;
    }
    switch (type.mapping.gamepadAxis)
    {
    case Gamepad_LeftStickX:   return range.scale(port, GAMECUBE_AXIS_STICK_X);
    case Gamepad_LeftStickY:   return range.scale(port, GAMECUBE_AXIS_STICK_Y);
    case Gamepad_RightStickX:  return range.scale(port, GAMECUBE_AXIS_C_STICK_X);
    case Gamepad_RightStickY:  return range.scale(port, GAMECUBE_AXIS_C_STICK_Y);
    // the click at the bottom of a trigger is all the way in, whatever the analog part says
    case Gamepad_LeftTrigger:  return port.buttons & GAMECUBE_BUTTON_L ? UINT16_MAX : range.scale(port, GAMECUBE_AXIS_L);
    case Gamepad_RightTrigger: return port.buttons & GAMECUBE_BUTTON_R ? UINT16_MAX : range.scale(port, GAMECUBE_AXIS_R);
    default:                   return 0;
    }
}

// Nintendo adapter rumble, one byte per port. Ports without the power cable, or with a WaveBird, can't
static inline void gamecube_adapter_rumble_report(const bool rumble[GAMECUBE_ADAPTER_PORTS],
                                                  const GameCubePortState ports[GAMECUBE_ADAPTER_PORTS],
                                                  uint8_t out[GAMECUBE_ADAPTER_RUMBLE_REPORT_LEN])
{
    out[0] = GAMECUBE_ADAPTER_RUMBLE_REPORT_ID;
    for (uint8_t i = 0; i < GAMECUBE_ADAPTER_PORTS; i++)
    {
        out[1 + i] = rumble[i] && ports[i].connected && ports[i].rumble_power && !ports[i].wireless;
    }
}

static inline void evoretro_rumble_report(bool rumble, uint8_t out[EVORETRO_RUMBLE_REPORT_LEN])
{
    out[0] = out[1] = rumble ? 0xFF : 0x00;
}
