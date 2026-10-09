#include "devices/joybus.hpp"
#include "events.pb.h"
#include "emulation/usb/hid_device.h"
#include <cstring>

JoybusDevice::JoybusDevice(proto_JoybusDevice device, uint16_t id)
    : Device(id), m_device(device), m_controller(device.dataPin)
{
}

bool JoybusDevice::matches_reload_config(const proto_Device &config) const
{
    return config.which_device == proto_Device_joybus_tag &&
           memcmp(&m_device, &config.device.joybus, sizeof(m_device)) == 0;
}

void JoybusDevice::begin()
{
    if (!m_controller.begin())
    {
        printf("Joybus: no free PIO state machine or DMA channel\r\n");
    }
}

void JoybusDevice::end(bool full)
{
    m_controller.end();
}

JoybusControllerType JoybusDevice::controller_type() const
{
    switch (m_controller.kind())
    {
    case JoybusControllerKind::N64:
        return JoybusControllerN64;
    case JoybusControllerKind::GameCube:
        return JoybusControllerGameCube;
    default:
        return JoybusControllerNone;
    }
}

void JoybusDevice::update(bool full_poll, bool send_events)
{
    m_controller.tick();
    auto type = controller_type();
    if (type != m_last_type || full_poll)
    {
        m_last_type = type;
        proto_Event event = {which_event : proto_Event_joybus_tag, event : {joybus : {m_id, type}}};
        HIDConfigDevice::send_event(event, true);
    }
}

bool JoybusDevice::using_pin(uint8_t pin)
{
    return pin == m_device.dataPin;
}

// N64 sticks are signed and centered on 0, inputs want 0 - 65535 centered on 32768
static uint16_t n64_axis(uint8_t raw)
{
    int32_t value = (int8_t)raw + 128;
    return std::min<int32_t>(std::max<int32_t>(value, 0), 255) << 8;
}

// The N64's C buttons double as a right stick
static uint16_t digital_axis(bool low, bool high)
{
    if (low == high)
    {
        return 0x8000;
    }
    return high ? UINT16_MAX : 0;
}

uint16_t JoybusDevice::read_axis(proto_JoybusAxisType type)
{
    const uint8_t *state = m_controller.state();
    switch (m_controller.kind())
    {
    case JoybusControllerKind::GameCube:
        switch (type)
        {
        case JoybusAxisStickX:
            return state[2] << 8;
        case JoybusAxisStickY:
            return state[3] << 8;
        case JoybusAxisCStickX:
            return state[4] << 8;
        case JoybusAxisCStickY:
            return state[5] << 8;
        case JoybusAxisLeftTrigger:
            return state[6] << 8;
        case JoybusAxisRightTrigger:
            return state[7] << 8;
        default:
            return 0;
        }
    case JoybusControllerKind::N64:
        switch (type)
        {
        case JoybusAxisStickX:
            return n64_axis(state[2]);
        case JoybusAxisStickY:
            return n64_axis(state[3]);
        case JoybusAxisCStickX:
            return digital_axis(state[1] & N64_MASK_C_LEFT, state[1] & N64_MASK_C_RIGHT);
        case JoybusAxisCStickY:
            return digital_axis(state[1] & N64_MASK_C_DOWN, state[1] & N64_MASK_C_UP);
        case JoybusAxisLeftTrigger:
            return (state[1] & N64_MASK_L) ? UINT16_MAX : 0;
        case JoybusAxisRightTrigger:
            return (state[1] & N64_MASK_R) ? UINT16_MAX : 0;
        default:
            return 0;
        }
    default:
        return 0;
    }
}

bool JoybusDevice::read_button(proto_JoybusButtonType type)
{
    const uint8_t *state = m_controller.state();
    switch (m_controller.kind())
    {
    case JoybusControllerKind::GameCube:
        switch (type)
        {
        case JoybusButtonA:
            return state[0] & GC_MASK_A;
        case JoybusButtonB:
            return state[0] & GC_MASK_B;
        case JoybusButtonX:
            return state[0] & GC_MASK_X;
        case JoybusButtonY:
            return state[0] & GC_MASK_Y;
        case JoybusButtonStart:
            return state[0] & GC_MASK_START;
        case JoybusButtonZ:
            return state[1] & GC_MASK_Z;
        case JoybusButtonL:
            return state[1] & GC_MASK_L;
        case JoybusButtonR:
            return state[1] & GC_MASK_R;
        case JoybusButtonDpadUp:
            return state[1] & GC_MASK_DPAD_UP;
        case JoybusButtonDpadDown:
            return state[1] & GC_MASK_DPAD_DOWN;
        case JoybusButtonDpadLeft:
            return state[1] & GC_MASK_DPAD_LEFT;
        case JoybusButtonDpadRight:
            return state[1] & GC_MASK_DPAD_RIGHT;
        default:
            return false;
        }
    case JoybusControllerKind::N64:
        switch (type)
        {
        case JoybusButtonA:
            return state[0] & N64_MASK_A;
        case JoybusButtonB:
            return state[0] & N64_MASK_B;
        case JoybusButtonStart:
            return state[0] & N64_MASK_START;
        case JoybusButtonZ:
            return state[0] & N64_MASK_Z;
        case JoybusButtonL:
            return state[1] & N64_MASK_L;
        case JoybusButtonR:
            return state[1] & N64_MASK_R;
        case JoybusButtonDpadUp:
            return state[0] & N64_MASK_DPAD_UP;
        case JoybusButtonDpadDown:
            return state[0] & N64_MASK_DPAD_DOWN;
        case JoybusButtonDpadLeft:
            return state[0] & N64_MASK_DPAD_LEFT;
        case JoybusButtonDpadRight:
            return state[0] & N64_MASK_DPAD_RIGHT;
        case JoybusButtonCUp:
            return state[1] & N64_MASK_C_UP;
        case JoybusButtonCDown:
            return state[1] & N64_MASK_C_DOWN;
        case JoybusButtonCLeft:
            return state[1] & N64_MASK_C_LEFT;
        case JoybusButtonCRight:
            return state[1] & N64_MASK_C_RIGHT;
        default:
            return false;
        }
    default:
        return false;
    }
}

void JoybusDevice::set_rumble(uint8_t left, uint8_t right)
{
    m_controller.set_rumble(left || right);
}

bool JoybusDevice::has_rumble() const
{
    return m_controller.kind() != JoybusControllerKind::None;
}
