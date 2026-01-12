#include "usb/device/switch_device.h"
#include "protocols/ps4.hpp"
#include "enums.pb.h"
#include "hid_reports.h"
#include "usb/usb_devices.h"

uint8_t const desc_hid_report_ps3[] =
    {
        TUD_HID_REPORT_DESC_PS3_THIRDPARTY_GAMEPAD()};
SwitchGamepadDevice::SwitchGamepadDevice()
{
}
void SwitchGamepadDevice::initialize()
{
}
void SwitchGamepadDevice::process(bool full_poll)
{
    if (!tud_ready() || usbd_edpt_busy(TUD_OPT_RHPORT, m_epin))
        return;
    for (const auto &mapping : mappings)
    {
        mapping->update(full_poll);
        mapping->update_switch(epin_buf);
    }
    SwitchProGamepad_Data_t *gamepad = (SwitchProGamepad_Data_t *)epin_buf;
    gamepad->leftStickX = PS3_STICK_CENTER;
    gamepad->leftStickY = PS3_STICK_CENTER;
    gamepad->rightStickX = PS3_STICK_CENTER;
    gamepad->rightStickY = PS3_STICK_CENTER;
    // convert bitmask dpad to actual hid dpad
    gamepad->dpad = dpad_bindings[gamepad->dpad];

    if (!usbd_edpt_claim(TUD_OPT_RHPORT, m_epin))
    {
        return;
    }
    usbd_edpt_xfer(TUD_OPT_RHPORT, m_epin, epin_buf, sizeof(SwitchProGamepad_Data_t));
}

size_t SwitchGamepadDevice::compatible_section_descriptor(uint8_t *dest, size_t remaining)
{
    return 0;
}

size_t SwitchGamepadDevice::config_descriptor(uint8_t *dest, size_t remaining)
{
    if (!m_eps_assigned)
    {
        m_eps_assigned = true;
        m_epin = next_epin();
        m_epout = next_epin();
    }
    uint8_t desc[] = {TUD_HID_INOUT_DESCRIPTOR(m_interface, 0, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report_ps3), m_epout, m_epin, CFG_TUD_HID_EP_BUFSIZE, 1)};
    assert(sizeof(desc) <= remaining);
    memcpy(dest, desc, sizeof(desc));
    return sizeof(desc);
}

void SwitchGamepadDevice::device_descriptor(tusb_desc_device_t *desc)
{
    // TODO: emulate an actual switch procon maybe?
    desc->idVendor = HORI_VID;
    desc->idProduct = HORI_POKKEN_TOURNAMENT_DX_PRO_PAD_PID;
}
const uint8_t *SwitchGamepadDevice::report_descriptor()
{
    return desc_hid_report_ps3;
}

uint16_t SwitchGamepadDevice::report_desc_len()
{
    return sizeof(desc_hid_report_ps3);
}
uint16_t SwitchGamepadDevice::get_report(uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
{
    printf("get report: %02x %02x %04x\r\n", report_id, report_type, reqlen);
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)reqlen;
    if (report_type != HID_REPORT_TYPE_FEATURE)
    {
        return 0;
    }

    return 0;
}

void SwitchGamepadDevice::set_report(uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize)
{
    switch (report_type)
    {
    case HID_REPORT_TYPE_FEATURE:
        switch (report_id)
        {
        case 0:

            return;
        }
    case HID_REPORT_TYPE_OUTPUT:
    {
        break;
    }
    default:
        break;
    }
}