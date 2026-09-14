#include "devices/bt/bt_ble_host.hpp"
#include "devices/bt/bt_host.hpp"
#include "managers/device_manager.hpp"
#include "hidparser.h"
#include "btstack.h"
#include "btstack_config.h"
#include "utils.h"

#include <memory>

#define XBOX_SERIES_VID  0x045E
#define XBOX_SERIES_PID  0x0B13   // Xbox Series X/S BLE PID

// ============================================================================
// BleGenericHost (HID-over-GATT / HIDS fallback & Xbox Series BLE)
// ============================================================================

BleGenericHost::~BleGenericHost()
{
    if (m_info)
    {
        USB_FreeReportInfo(m_info);
        m_info = nullptr;
    }
}

void BleGenericHost::handle_report(const uint8_t *data, uint16_t len)
{
    BluetoothHostInterface::handle_report(data, len);
    m_data = {};
    fill_generic_report(m_info, m_report_buf, &m_data);
}

bool BleGenericHost::tick_digital(proto_Output &type)
{
    if (type.which_mapping == proto_Output_gamepadButton_tag)
    {
        switch (type.mapping.gamepadButton)
        {
        case Gamepad_A:               return (m_data.genericButtons & (1 << 0)) != 0;
        case Gamepad_B:               return (m_data.genericButtons & (1 << 1)) != 0;
        case Gamepad_X:               return (m_data.genericButtons & (1 << 2)) != 0;
        case Gamepad_Y:               return (m_data.genericButtons & (1 << 3)) != 0;
        case Gamepad_LeftShoulder:    return (m_data.genericButtons & (1 << 4)) != 0;
        case Gamepad_RightShoulder:   return (m_data.genericButtons & (1 << 5)) != 0;
        case Gamepad_Back:            return (m_data.genericButtons & (1 << 6)) != 0;
        case Gamepad_Start:           return (m_data.genericButtons & (1 << 7)) != 0;
        case Gamepad_LeftThumbClick:  return (m_data.genericButtons & (1 << 8)) != 0;
        case Gamepad_RightThumbClick: return (m_data.genericButtons & (1 << 9)) != 0;
        case Gamepad_Guide:           return (m_data.genericButtons & (1 << 10)) != 0;
        case Gamepad_Capture:         return (m_data.genericButtons & (1 << 11)) != 0;
        case Gamepad_DpadUp:          return m_data.dpadUp != 0;
        case Gamepad_DpadDown:        return m_data.dpadDown != 0;
        case Gamepad_DpadLeft:        return m_data.dpadLeft != 0;
        case Gamepad_DpadRight:       return m_data.dpadRight != 0;
        default:                      return false;
        }
    }
    return false;
}

uint16_t BleGenericHost::tick_analog(proto_Output &type)
{
    if (type.which_mapping == proto_Output_gamepadAxis_tag)
    {
        switch (type.mapping.gamepadAxis)
        {
        case Gamepad_LeftStickX:   return m_data.genericAxisX;
        case Gamepad_LeftStickY:   return m_data.genericAxisY;
        case Gamepad_RightStickX:  return m_data.genericAxisRx;
        case Gamepad_RightStickY:  return m_data.genericAxisRy;
        case Gamepad_LeftTrigger:  return m_data.genericAxisZ;
        case Gamepad_RightTrigger: return m_data.genericAxisRz;
        default:                   return 0;
        }
    }
    return 0;
}

// ============================================================================
// BleGhlIosHost (Guitar Hero Live iOS BLE Guitar)
// ============================================================================

bool BleGhlIosHost::tick_digital(proto_Output &type)
{
    uint8_t frets   = m_report_buf[0];
    uint8_t buttons = m_report_buf[1];
    uint8_t strum   = m_report_buf[4];

    if (type.which_mapping == proto_Output_ghlButton_tag)
    {
        switch (type.mapping.ghlButton)
        {
        case GuitarHeroLiveGuitar_Black1:    return (frets & 0x02) != 0;
        case GuitarHeroLiveGuitar_Black2:    return (frets & 0x04) != 0;
        case GuitarHeroLiveGuitar_Black3:    return (frets & 0x08) != 0;
        case GuitarHeroLiveGuitar_White1:    return (frets & 0x01) != 0;
        case GuitarHeroLiveGuitar_White2:    return (frets & 0x10) != 0;
        case GuitarHeroLiveGuitar_White3:    return (frets & 0x20) != 0;
        case GuitarHeroLiveGuitar_StrumUp:   return strum == 0x00;
        case GuitarHeroLiveGuitar_StrumDown: return strum == 0xFF;
        case GuitarHeroLiveGuitar_GHTV:      return (buttons & 0x04) != 0;
        default:                             return false;
        }
    }
    if (type.which_mapping == proto_Output_gamepadButton_tag)
    {
        switch (type.mapping.gamepadButton)
        {
        case Gamepad_A:               return (frets & 0x02) != 0; // Black 1
        case Gamepad_B:               return (frets & 0x04) != 0; // Black 2
        case Gamepad_Y:               return (frets & 0x08) != 0; // Black 3
        case Gamepad_X:               return (frets & 0x01) != 0; // White 1
        case Gamepad_LeftShoulder:    return (frets & 0x10) != 0; // White 2
        case Gamepad_RightShoulder:   return (frets & 0x20) != 0; // White 3
        case Gamepad_Start:           return (buttons & 0x02) != 0; // Pause
        case Gamepad_Back:            return (buttons & 0x08) != 0; // Hero Power
        case Gamepad_LeftThumbClick:  return (buttons & 0x04) != 0; // GHTV
        case Gamepad_Guide:           return (buttons & 0x10) != 0; // Sync
        case Gamepad_DpadUp:          return strum == 0x00;
        case Gamepad_DpadDown:        return strum == 0xFF;
        default:                      return false;
        }
    }
    return false;
}

uint16_t BleGhlIosHost::tick_analog(proto_Output &type)
{
    uint8_t tilt   = m_report_buf[6];
    uint8_t whammy = m_report_buf[19];

    if (type.which_mapping == proto_Output_ghlAxis_tag)
    {
        switch (type.mapping.ghlAxis)
        {
        case GuitarHeroLiveGuitar_Tilt:   return (uint16_t)tilt * 0x101;
        case GuitarHeroLiveGuitar_Whammy: return (uint16_t)whammy * 0x101;
        default:                          return 0;
        }
    }
    if (type.which_mapping == proto_Output_gamepadAxis_tag)
    {
        switch (type.mapping.gamepadAxis)
        {
        case Gamepad_RightStickY: return (uint16_t)tilt * 0x101;
        case Gamepad_RightStickX: return (uint16_t)whammy * 0x101;
        default:                  return 0;
        }
    }
    return 0;
}

// ============================================================================
// Factory
// ============================================================================

std::shared_ptr<BluetoothHostInterface> ble_create_host(uint16_t vid, uint16_t pid,
                                                         uint16_t version,
                                                         uint16_t device_id,
                                                         HID_ReportInfo_t *info,
                                                         SubType known_subtype)
{
    // Xbox Series X/S / Xbox One BLE controllers (VID 0x045E) and any other
    // standard HID-over-GATT gamepads use BleGenericHost with report parsing via fill_generic_report.
    auto host = std::make_shared<BleGenericHost>(device_id, info);
    if (known_subtype != SubType_Unknown)
    {
        host->m_subtype = known_subtype;
    }
    host->m_vid = vid;
    host->m_pid = pid;
    return host;
}
