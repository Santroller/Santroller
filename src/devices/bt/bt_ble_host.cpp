#include "devices/bt/bt_ble_host.hpp"
#include "devices/bt/bt_host.hpp"
#include "devices/bt/bluetooth_stack.hpp"
#include "managers/device_manager.hpp"
#include "hidparser.h"
#include "btstack.h"
#include "btstack_config.h"
#include "utils.h"

#include <memory>

#include "ble/gatt-service/hids_host.h"
#include "devices/usb/host/xinput_tick_helpers.h"
#include "devices/usb/host/generic_hid_tick_helpers.h"
#include "protocols/hid.hpp"
#include "protocols/xinput.hpp"
#include "protocols/santroller_v1.hpp"
#include "protocols/santroller_v2.hpp"
#include "protocols/switch.hpp"

bool switch_tick_digital(const uint8_t *buf, proto_Output &type);
uint16_t switch_tick_analog(const uint8_t *buf, proto_Output &type);

#define XBOX_SERIES_VID  0x045E
#define XBOX_SERIES_PID  0x0B13   // Xbox Series X/S BLE PID

#define ARDWIINO_VID     0x1209
#define ARDWIINO_PID     0x2882
#define ARDWIINO_PID_BLE 0x2885

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
    fill_generic_report(m_info, m_report_buf, &m_data);
}

bool BleGenericHost::tick_digital(proto_Output &type)
{
    return generic_hid_tick_digital_impl(m_data, type);
}

uint16_t BleGenericHost::tick_analog(proto_Output &type)
{
    return generic_hid_tick_analog_impl(m_data, type);
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
// BleSantrollerHost (Santroller 1 & 2 BLE Host)
// ============================================================================

BleSantrollerHost::BleSantrollerHost(uint16_t id, bool is_v2, uint16_t version, SubType known_subtype)
    : BluetoothHostInterface(id), m_is_v2(is_v2)
{
    m_output.is_v2 = is_v2;
    if (m_is_v2)
    {
        if (known_subtype != SubType_Unknown)
        {
            m_subtype = known_subtype;
            m_ready = true;
        }
        else
        {
            m_subtype = SubType_Unknown;
            m_ready = false;
        }
    }
    else
    {
        // Santroller 1: SubType is encoded in version high byte: (version >> 8) & 0xFF
        uint8_t st = (version >> 8) & 0xFF;
        if (known_subtype != SubType_Unknown)
        {
            m_subtype = known_subtype;
        }
        else if (st != 0)
        {
            m_subtype = (SubType)st;
        }
        else
        {
            m_subtype = SubType_Gamepad;
        }
        m_ready = true;
    }
}

void BleSantrollerHost::on_connected()
{
    BluetoothHostInterface::on_connected();
    m_output.mark_all_dirty();
    if (m_is_v2)
    {
        if (m_subtype != SubType_Unknown)
        {
            set_ready(true);
        }
        // Query Santroller capabilities (Report 0x10) over GATT. Even with the subtype
        // already known from pairing, the capability bits decide which outputs to send.
        hids_host_send_get_report(m_cid, ReportIdSantrollerCapabilities, HID_REPORT_TYPE_INPUT);
        static const uint8_t cmd[2] = {ReportIdSantrollerCapabilities, 0};
        hids_host_send_write_report(m_cid, ReportIdSantrollerCapabilities, HID_REPORT_TYPE_OUTPUT, cmd, sizeof(cmd));
    }
    else
    {
        set_ready(true);
    }
}

void BleSantrollerHost::handle_report(const uint8_t *data, uint16_t len)
{
    if (m_is_v2)
    {
        handle_report_v2(data, len);
    }
    else
    {
        handle_report_v1(data, len);
    }
}

void BleSantrollerHost::handle_report_v2(const uint8_t *data, uint16_t len)
{
    if (len == 0) return;

    if (data[0] == ReportIdSantrollerCapabilities)
    {
        if (len >= 2)
        {
            m_subtype = (SubType)data[1];
            if (len >= 3)
            {
                m_capabilities = data[2];
            }
            m_output.subtype = m_subtype;
            m_output.capabilities = m_capabilities;
            m_output.mark_all_dirty();
            if (!m_ready)
            {
                set_ready(true);
            }
        }
        return;
    }

    if (data[0] != 1) return;

    if (!m_ready)
    {
        if (++m_query_attempts % 30 == 1)
        {
            hids_host_send_get_report(m_cid, ReportIdSantrollerCapabilities, HID_REPORT_TYPE_INPUT);
            static const uint8_t cmd[2] = {ReportIdSantrollerCapabilities, 0};
            hids_host_send_write_report(m_cid, ReportIdSantrollerCapabilities, HID_REPORT_TYPE_OUTPUT, cmd, sizeof(cmd));
        }
        return;
    }

    uint16_t copy_len = len < sizeof(m_report_buf) ? len : sizeof(m_report_buf);
    memcpy(m_report_buf, data, copy_len);
    santroller_v2_normalize_report(m_report_buf, copy_len, m_subtype);
}

void BleSantrollerHost::handle_report_v1(const uint8_t *data, uint16_t len)
{
    if (len == 0) return;
    uint16_t copy_len = len < sizeof(m_report_buf) ? len : sizeof(m_report_buf);
    memcpy(m_report_buf, data, copy_len);
}

void BleSantrollerHost::update(bool full_poll, bool send_events)
{
    BluetoothHostInterface::update(full_poll, send_events);
    if (!m_ready || !m_cid)
        return;
    sync_output();
    // one pending rumble / LED / stage kit command per pass. A HOGP report write carries
    // the report id in the characteristic, so the payload starts at the command byte.
    uint8_t buf[8];
    uint8_t len = m_output.peek(buf, sizeof(buf));
    if (!len)
        return;
    uint8_t status;
    {
        BtStackLock lock;
        status = hids_host_send_write_report(m_cid, ReportIdGamepad, HID_REPORT_TYPE_OUTPUT, buf + 1, len - 1);
    }
    if (status == ERROR_CODE_SUCCESS)
        m_output.commit(buf[1]);
}

bool BleSantrollerHost::tick_digital(proto_Output &type)
{
    if (m_is_v2)
    {
        return xinput_tick_digital_impl(m_report_buf, m_subtype, type);
    }
    return santroller_v1_tick_digital(m_report_buf, m_subtype, type);
}

uint16_t BleSantrollerHost::tick_analog(proto_Output &type)
{
    if (m_is_v2)
    {
        return xinput_tick_analog_impl(m_report_buf, m_subtype, type);
    }
    return santroller_v1_tick_analog(m_report_buf, m_subtype, type);
}

uint16_t BleSantrollerHost::tick_button_pressure(proto_Output &type)
{
    if (m_is_v2)
        return xinput_tick_button_pressure_impl(m_report_buf, m_subtype, type);
    return tick_digital(type) ? UINT16_MAX : 0;
}


// ============================================================================
// BleSteamHost (Valve Steam Controller BLE)
// ============================================================================

BleSteamHost::BleSteamHost(uint16_t id) : BluetoothHostInterface(id)
{
    m_subtype = SubType_Gamepad;
}

void BleSteamHost::on_connected()
{
    BluetoothHostInterface::on_connected();
    if (m_cid)
    {
        hids_host_send_write_report(m_cid, 0, HID_REPORT_TYPE_FEATURE,
                                    (uint8_t *)STEAM_CMD_CLEAR_MAPPINGS_BUF, sizeof(STEAM_CMD_CLEAR_MAPPINGS_BUF));
        hids_host_send_write_report(m_cid, 0, HID_REPORT_TYPE_FEATURE,
                                    (uint8_t *)STEAM_CMD_DISABLE_LIZARD_BUF, sizeof(STEAM_CMD_DISABLE_LIZARD_BUF));
    }
    if (m_con_handle && m_char_handle)
    {
        gatt_client_write_value_of_characteristic_without_response(
            m_con_handle, m_char_handle, sizeof(STEAM_CMD_CLEAR_MAPPINGS_BUF),
            (uint8_t *)STEAM_CMD_CLEAR_MAPPINGS_BUF);
        gatt_client_write_value_of_characteristic_without_response(
            m_con_handle, m_char_handle, sizeof(STEAM_CMD_DISABLE_LIZARD_BUF),
            (uint8_t *)STEAM_CMD_DISABLE_LIZARD_BUF);
    }
    set_ready(true);
}

void BleSteamHost::handle_report(const uint8_t *data, uint16_t len)
{
    if (len == 0) return;
    uint16_t copy_len = len < sizeof(m_report_buf) ? len : sizeof(m_report_buf);
    memcpy(m_report_buf, data, copy_len);

    steam_parse_ble_report(data, len, m_state);
}

bool BleSteamHost::tick_digital(proto_Output &type)
{
    return steam_tick_digital(m_state, type);
}

uint16_t BleSteamHost::tick_analog(proto_Output &type)
{
    return steam_tick_analog(m_state, type);
}

// ============================================================================
// BleSwitch2Host (Switch 2 BLE Host)
// ============================================================================

BleSwitch2Host::BleSwitch2Host(uint16_t id) : BluetoothHostInterface(id)
{
    m_subtype = SubType_Gamepad;
    set_ready(true);
}

void BleSwitch2Host::handle_report(const uint8_t *data, uint16_t len)
{
    if (len == 0) return;
    uint16_t copy_len = len < sizeof(m_report_buf) ? len : sizeof(m_report_buf);
    memcpy(m_report_buf, data, copy_len);

    switch2_parse_report(data, len, m_state);
}

bool BleSwitch2Host::tick_digital(proto_Output &type)
{
    if (m_report_buf[0] == SWITCH_PRO_CON_FULL_REPORT_ID || m_report_buf[0] == 0x21 || m_report_buf[0] == 0x3F)
    {
        return switch_tick_digital(m_report_buf, type);
    }
    return switch2_tick_digital(m_state, type);
}

uint16_t BleSwitch2Host::tick_analog(proto_Output &type)
{
    if (m_report_buf[0] == SWITCH_PRO_CON_FULL_REPORT_ID || m_report_buf[0] == 0x21 || m_report_buf[0] == 0x3F)
    {
        return switch_tick_analog(m_report_buf, type);
    }
    return switch2_tick_analog(m_state, type);
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
    bool is_santroller = (vid == ARDWIINO_VID && (pid == ARDWIINO_PID || pid == ARDWIINO_PID_BLE));

    if (is_santroller)
    {
        auto host = std::make_shared<BleSantrollerHost>(device_id, info->foundSantrollerV2OutputUsage, version, known_subtype);
        host->m_vid = vid ? vid : ARDWIINO_VID;
        host->m_pid = pid ? pid : (info->foundSantrollerV2OutputUsage ? ARDWIINO_PID : ARDWIINO_PID_BLE);
        return host;
    }

    // Valve Steam Controller / HORI Steam Controller
    if (vid == VALVE_USB_VID || (vid == HORI_VID && pid == HORI_STEAM_CONTROLLER_PID))
    {
        auto host = std::make_shared<BleSteamHost>(device_id);
        host->m_vid = vid;
        host->m_pid = pid;
        if (known_subtype != SubType_Unknown)
        {
            host->m_subtype = known_subtype;
        }
        return host;
    }

    // Switch 2 BLE Controllers
    if (vid == NINTENDO_VID && (pid == SWITCH_2_PRO_PID || pid == SWITCH_2_JOY_L_PID ||
                                pid == SWITCH_2_JOY_R_PID || pid == SWITCH_2_GC_PID))
    {
        auto host = std::make_shared<BleSwitch2Host>(device_id);
        host->m_vid = vid;
        host->m_pid = pid;
        return host;
    }

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
