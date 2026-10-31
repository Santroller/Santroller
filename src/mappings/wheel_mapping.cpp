#include "mappings/mapping.hpp"
#include "protocols/wheel.hpp"

static proto_Mapping wheel_as_gamepad(const proto_Mapping &mapping)
{
    proto_Mapping converted = mapping;
    // the clutch and handbrake rest at their minimum on the sticks, which the XInput and HID
    // devices start their wheel reports at
    wheel_axis_as_gamepad(converted.mapping);
    return converted;
}

WheelAxisMapping::WheelAxisMapping(const proto_Mapping &mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : WheelAxisMapping(wheel_as_gamepad(mapping), mapping.mapping.mapping.wheelAxis, std::move(input), id, profile)
{
}

// Everything but the steering is a pedal or lever, so it calibrates like a trigger
WheelAxisMapping::WheelAxisMapping(const proto_Mapping &gamepad_mapping, WheelAxisType axis, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : GamepadAxisMapping(gamepad_mapping, std::move(input), id, profile, axis != Wheel_Steering), m_axis(axis)
{
}

// PC sims bind whatever axes they find, so the pedals get the 16 bit axes instead of 8 bit
// triggers, resting at the minimum (see HIDGamepadDevice::initialize). The handbrake is fine on a trigger.
void WheelAxisMapping::update_hid(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    PCGamepadDpad_Data_t *report = (PCGamepadDpad_Data_t *)buf;
    int16_t value = static_cast<int32_t>(m_calibrated_value) - 32768;
    switch (m_axis)
    {
    case Wheel_Steering:
        report->leftStickX = value;
        break;
    case Wheel_Accelerator:
        report->leftStickY = value;
        break;
    case Wheel_Brake:
        report->rightStickX = value;
        break;
    case Wheel_Clutch:
        report->rightStickY = value;
        break;
    case Wheel_Handbrake:
        report->leftTrigger = m_calibrated_value >> 8;
        break;
    default:
        break;
    }
}

void WheelAxisMapping::update_wii(uint8_t format, uint8_t *buf)
{
    if (on_gamepad())
    {
        GamepadAxisMapping::update_wii(format, buf);
    }
}

void WheelAxisMapping::update_switch(uint8_t *buf)
{
    if (on_gamepad())
    {
        GamepadAxisMapping::update_switch(buf);
    }
}

void WheelAxisMapping::update_ps2(uint8_t *buf)
{
    if (on_gamepad())
    {
        GamepadAxisMapping::update_ps2(buf);
    }
}

// A GameCube wheel is a Speed Force, with 8 bit steering in the stick's Y and the pedals (released
// at 0) on the C stick
void WheelAxisMapping::update_gamecube(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    GameCubeGamepad_Data_t *report = (GameCubeGamepad_Data_t *)buf;
    switch (m_axis)
    {
    case Wheel_Steering:
        report->leftStickY = m_calibrated_value >> 8;
        break;
    case Wheel_Accelerator:
        report->cStickX = m_calibrated_value >> 8;
        break;
    case Wheel_Brake:
        report->cStickY = m_calibrated_value >> 8;
        break;
    default:
        break;
    }
}

void WheelAxisMapping::update_n64(uint8_t *buf)
{
    if (on_gamepad())
    {
        GamepadAxisMapping::update_n64(buf);
    }
}

// A PS3 wheel is a G27, with 14 bit steering and 8 bit pedals (that read 0xFF released)
void WheelAxisMapping::update_ps3(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    PS3G27_Data_t *report = (PS3G27_Data_t *)buf;
    switch (m_axis)
    {
    case Wheel_Steering:
        report->steering = m_calibrated_value >> 2;
        break;
    case Wheel_Accelerator:
        report->accelerator = UINT8_MAX - (m_calibrated_value >> 8);
        break;
    case Wheel_Brake:
        report->brake = UINT8_MAX - (m_calibrated_value >> 8);
        break;
    case Wheel_Clutch:
        report->clutch = UINT8_MAX - (m_calibrated_value >> 8);
        break;
    default:
        break;
    }
}

// PS4 wheels are G29s, which have their own 16 bit steering and pedals (that read 0xFFFF
// released) but no handbrake. PS5 wheels are assumed to be the same.
static void update_ps_wheel(PSWheel_Data_t &wheel, WheelAxisType axis, uint16_t value)
{
    switch (axis)
    {
    case Wheel_Steering:
        wheel.steering = value;
        break;
    case Wheel_Accelerator:
        wheel.accelerator = UINT16_MAX - value;
        break;
    case Wheel_Brake:
        wheel.brake = UINT16_MAX - value;
        break;
    case Wheel_Clutch:
        wheel.clutch = UINT16_MAX - value;
        break;
    default:
        break;
    }
}

void WheelAxisMapping::update_ps4(uint8_t *buf)
{
    if (!m_centered)
    {
        update_ps_wheel(((PS4Wheel_Data_t *)buf)->wheel, m_axis, m_calibrated_value);
    }
}

void WheelAxisMapping::update_ps5(uint8_t *buf)
{
    if (!m_centered)
    {
        update_ps_wheel(((PS5Wheel_Data_t *)buf)->wheel, m_axis, m_calibrated_value);
    }
}

void WheelAxisMapping::update_ogxbox(uint8_t *buf)
{
    if (on_gamepad())
    {
        GamepadAxisMapping::update_ogxbox(buf);
    }
}

// An Xbox One wheel is a G920, with 16 bit steering and 8 bit pedals but no handbrake
void WheelAxisMapping::update_xboxone(uint8_t *buf)
{
    if (m_centered)
    {
        return;
    }
    XboxOneWheel_Data_t *report = (XboxOneWheel_Data_t *)buf;
    switch (m_axis)
    {
    case Wheel_Steering:
        report->steering = m_calibrated_value;
        break;
    case Wheel_Accelerator:
        report->accelerator = m_calibrated_value >> 8;
        break;
    case Wheel_Brake:
        report->brake = m_calibrated_value >> 8;
        break;
    case Wheel_Clutch:
        report->clutch = m_calibrated_value >> 8;
        break;
    default:
        break;
    }
}

WheelButtonMapping::WheelButtonMapping(const proto_Mapping &mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : ButtonMapping(mapping, std::move(input), id, profile)
{
}

// Where each WheelButtonType is in PSWheel_Data_t, as its byte and bit
static const uint8_t ps_wheel_buttons[][2] = {
    {offsetof(PSWheel_Data_t, unknown) - 1, 0x01}, // gear 1
    {offsetof(PSWheel_Data_t, unknown) - 1, 0x02},
    {offsetof(PSWheel_Data_t, unknown) - 1, 0x04},
    {offsetof(PSWheel_Data_t, unknown) - 1, 0x08},
    {offsetof(PSWheel_Data_t, unknown) - 1, 0x10},
    {offsetof(PSWheel_Data_t, unknown) - 1, 0x20}, // gear 6
    {offsetof(PSWheel_Data_t, unknown) - 1, 0x80}, // reverse
    {sizeof(PSWheel_Data_t) - 1, 0x10},            // plus
    {sizeof(PSWheel_Data_t) - 1, 0x08},            // minus
    {sizeof(PSWheel_Data_t) - 1, 0x04},            // dial up
    {sizeof(PSWheel_Data_t) - 1, 0x02},            // dial down
    {sizeof(PSWheel_Data_t) - 1, 0x01},            // dial enter
};

static void update_ps_wheel(PSWheel_Data_t &wheel, WheelButtonType button, bool pressed)
{
    if (pressed && button >= Wheel_Gear1 && button <= Wheel_DialEnter)
    {
        const uint8_t *bit = ps_wheel_buttons[button - Wheel_Gear1];
        ((uint8_t *)&wheel)[bit[0]] |= bit[1];
    }
}

void WheelButtonMapping::update_ps4(uint8_t *buf)
{
    update_ps_wheel(((PS4Wheel_Data_t *)buf)->wheel, m_mapping.mapping.mapping.wheelButton, m_last_value);
}

void WheelButtonMapping::update_ps5(uint8_t *buf)
{
    update_ps_wheel(((PS5Wheel_Data_t *)buf)->wheel, m_mapping.mapping.mapping.wheelButton, m_last_value);
}

// An Xbox One wheel reports the gear it is in, rather than a bit for each one
void WheelButtonMapping::update_xboxone(uint8_t *buf)
{
    if (!m_last_value)
    {
        return;
    }
    XboxOneWheel_Data_t *report = (XboxOneWheel_Data_t *)buf;
    WheelButtonType button = m_mapping.mapping.mapping.wheelButton;
    if (button >= Wheel_Gear1 && button <= Wheel_Gear6)
    {
        report->gear = button - Wheel_Gear1 + 1;
    }
    else if (button == Wheel_GearReverse)
    {
        report->gear = -1;
    }
}

WheelGamepadAxisMapping::WheelGamepadAxisMapping(const proto_Mapping &mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : GamepadAxisMapping(mapping, std::move(input), id, profile)
{
}

// The wheel's data is where an Xbox One gamepad keeps its sticks
void WheelGamepadAxisMapping::update_xboxone(uint8_t *buf)
{
    (void)buf;
}

// A G27's H pattern, where the shifter's stick sits in each gear (from GIMX)
static const uint8_t g27_gear_positions[][2] = {
    {G27_SHIFTER_LEFT, G27_SHIFTER_UP},
    {G27_SHIFTER_LEFT, G27_SHIFTER_DOWN},
    {G27_SHIFTER_CENTER, G27_SHIFTER_UP},
    {G27_SHIFTER_CENTER, G27_SHIFTER_DOWN},
    {G27_SHIFTER_RIGHT, G27_SHIFTER_UP},
    {G27_SHIFTER_RIGHT, G27_SHIFTER_DOWN},
};

// A G27 reports the gear it is in as a button, and where its shifter's stick is. Reverse is past 6th,
// with the stick pushed down, and has no button
void WheelButtonMapping::update_ps3(uint8_t *buf)
{
    if (!m_last_value)
    {
        return;
    }
    PS3G27_Data_t *report = (PS3G27_Data_t *)buf;
    WheelButtonType button = m_mapping.mapping.mapping.wheelButton;
    if (button >= Wheel_Gear1 && button <= Wheel_Gear6)
    {
        uint8_t gear = button - Wheel_Gear1;
        // gears 1 - 6 are the low bits of byte 2
        ((uint8_t *)report)[2] |= 1 << gear;
        report->shifterX = g27_gear_positions[gear][0];
        report->shifterY = g27_gear_positions[gear][1];
    }
    else if (button == Wheel_GearReverse)
    {
        report->shifterX = G27_SHIFTER_RIGHT;
        report->shifterY = G27_SHIFTER_DOWN;
        report->shifterStatus |= G27_SHIFTER_REVERSE;
    }
}

void WheelGamepadAxisMapping::update_ps3(uint8_t *buf)
{
    (void)buf;
}

void WheelGamepadAxisMapping::update_gamecube(uint8_t *buf)
{
    (void)buf;
}

WheelGamepadButtonMapping::WheelGamepadButtonMapping(const proto_Mapping &mapping, std::unique_ptr<Input> input, uint16_t id, std::shared_ptr<Profile> profile) : GamepadButtonMapping(mapping, std::move(input), id, profile)
{
}

// A Speed Force's paddles are the analog triggers, fully pressed
void WheelGamepadButtonMapping::update_gamecube(uint8_t *buf)
{
    GameCubeGamepad_Data_t *report = (GameCubeGamepad_Data_t *)buf;
    switch (m_mapping.mapping.mapping.gamepadButton)
    {
    case Gamepad_LeftShoulder:
        report->leftTrigger |= m_last_value ? UINT8_MAX : 0;
        break;
    case Gamepad_RightShoulder:
        report->rightTrigger |= m_last_value ? UINT8_MAX : 0;
        break;
    default:
        GamepadButtonMapping::update_gamecube(buf);
        break;
    }
}

// Xbox racing games expect the paddles on A (gear down) and X (gear up), like a Mad Catz MC2
void WheelGamepadButtonMapping::update_ogxbox(uint8_t *buf)
{
    OGXboxGamepad_Data_t *report = (OGXboxGamepad_Data_t *)buf;
    uint8_t pressure = m_last_pressure >> 8;
    switch (m_mapping.mapping.mapping.gamepadButton)
    {
    case Gamepad_LeftShoulder:
        report->a = std::max(report->a, pressure);
        break;
    case Gamepad_RightShoulder:
        report->x = std::max(report->x, pressure);
        break;
    default:
        GamepadButtonMapping::update_ogxbox(buf);
        break;
    }
}

// A PS3 wheel is a G27, which has its own report and no PS button
void WheelGamepadButtonMapping::update_ps3(uint8_t *buf)
{
    PS3G27_Data_t *report = (PS3G27_Data_t *)buf;
    switch (m_mapping.mapping.mapping.gamepadButton)
    {
    case Gamepad_A:
        report->cross |= m_last_value;
        break;
    case Gamepad_B:
        report->circle |= m_last_value;
        break;
    case Gamepad_X:
        report->square |= m_last_value;
        break;
    case Gamepad_Y:
        report->triangle |= m_last_value;
        break;
    case Gamepad_LeftShoulder:
        report->l1 |= m_last_value;
        break;
    case Gamepad_RightShoulder:
        report->r1 |= m_last_value;
        break;
    case Gamepad_Back:
        report->select |= m_last_value;
        break;
    case Gamepad_Start:
        report->start |= m_last_value;
        break;
    case Gamepad_LeftThumbClick:
        report->l3 |= m_last_value;
        break;
    case Gamepad_RightThumbClick:
        report->r3 |= m_last_value;
        break;
    case Gamepad_DpadUp:
        report->dpadUp |= m_last_value;
        break;
    case Gamepad_DpadDown:
        report->dpadDown |= m_last_value;
        break;
    case Gamepad_DpadLeft:
        report->dpadLeft |= m_last_value;
        break;
    case Gamepad_DpadRight:
        report->dpadRight |= m_last_value;
        break;
    default:
        break;
    }
}
