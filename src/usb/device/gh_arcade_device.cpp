#include "tusb_option.h"
#include "class/hid/hid.h"
#include "common/tusb_common.h"
#include "device/usbd_pvt.h"
#include "usb/device/gh_arcade_device.h"
#include "hid_reports.h"
#include "config.hpp"

static uint8_t const desc_hid_report_arcade[] = {TUD_HID_REPORT_DESC_GUITAR_HERO_ARCADE()};
static char const str_gha_input[] = "RT-GH INPUT ";
static char const str_gha_led[] = "RT-GH LED ";
static char const str_gha_controller[] = "RT-GH CONTROLLER ";


GHArcadeVendorDevice::GHArcadeVendorDevice()
{
}
uint16_t GHArcadeVendorDevice::open(tusb_desc_interface_t const *itf_desc, uint16_t max_len)
{
    TU_VERIFY(TUSB_CLASS_VENDOR_SPECIFIC == itf_desc->bInterfaceClass && itf_desc->bInterfaceSubClass == 0x01 && itf_desc->bInterfaceProtocol == 0xFF, 0);
    uint16_t drv_len;
    drv_len = sizeof(tusb_desc_interface_t) +
              (itf_desc->bNumEndpoints * sizeof(tusb_desc_endpoint_t));

    TU_VERIFY(max_len >= drv_len, 0);
    uint8_t const *p_desc = (uint8_t const *)itf_desc;

    p_desc = tu_desc_next(p_desc);
    TU_VERIFY(usbd_open_edpt_pair(TUD_OPT_RHPORT, p_desc, itf_desc->bNumEndpoints,
                                  TUSB_XFER_INTERRUPT, &m_epout,
                                  &m_epin1),
              0);
    if (m_epout)
    {
        if (!usbd_edpt_xfer(TUD_OPT_RHPORT, m_epout, epout_buf, sizeof(epout_buf), false))
        {
            TU_LOG_FAILED();
            TU_BREAKPOINT();
        }
    }
    return drv_len;
}
void GHArcadeVendorDevice::initialize()
{
    m_epin1 = next_epin();
    m_epin2 = next_epin();
    m_epout = next_epin();
    m_strid = next_strid();
    usb_instances_by_epin[m_epin1 & (~0x80)] = usb_instances[interface_id];
    usb_instances_by_epout[m_epin2] = usb_instances[interface_id];
    usb_instances_by_epout[m_epout] = usb_instances[interface_id];
}
void GHArcadeVendorDevice::process()
{
    return;
}

size_t GHArcadeVendorDevice::compatible_section_descriptor(uint8_t *dest, size_t remaining)
{
    return 0;
}
size_t GHArcadeVendorDevice::device_name(uint8_t idx, char *desc) 
{
    if (m_strid != idx) {
        return 0;
    }
    memcpy(desc, str_gha_led, sizeof(str_gha_led));
    return sizeof(str_gha_led);
}

size_t GHArcadeVendorDevice::config_descriptor(uint8_t *dest, size_t remaining)
{
    uint8_t desc[] = {TUD_GHARCADE_VENDOR_DESCRIPTOR(interface_id, m_epin1, m_epout, m_epin2, m_strid)};
    assert(sizeof(desc) <= remaining);
    memcpy(dest, desc, sizeof(desc));
    return sizeof(desc);
}

void GHArcadeVendorDevice::device_descriptor(tusb_desc_device_t *desc)
{
    desc->idVendor = 0x0c70;
    desc->idProduct = 0x0777;
}

bool GHArcadeVendorDevice::interrupt_xfer(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
    if (epout_buf[0] == 0x03)
    {
        // packet sends 0 for left and 1 for right, hid report uses 1 for left and 2 for right
        side = epout_buf[1] + 1;
    }
    TU_VERIFY(usbd_edpt_xfer(TUD_OPT_RHPORT, m_epout, epout_buf, 0x40, false));
    return true;
}
bool GHArcadeVendorDevice::control_transfer(uint8_t stage, tusb_control_request_t const *request)
{
    return false;
}

GHArcadeGamepadDevice::GHArcadeGamepadDevice()
{
}
void GHArcadeGamepadDevice::initialize()
{
    m_strid = next_strid();
    m_epin = next_epin();
}
void GHArcadeGamepadDevice::process()
{
    if (!ready())
        return;
    ArcadeGuitarHeroGuitar_Data_t *report = (ArcadeGuitarHeroGuitar_Data_t *)epin_buf;
    memset(epin_buf, 0, sizeof(epin_buf));
    report->always_1d = 0x1d;
    report->always_ff = 0xff;
    for (const auto &profile : profiles)
    {
        for (const auto &mapping : profile->mappings)
        {
            mapping->update(false, false);
            mapping->update_hid(epin_buf);
        }
        for (const auto& led : profile->leds) {
            led->update(false, false);
        }
    }
    send_report(sizeof(XInputGamepad_Data_t), 0, epin_buf);
}

size_t GHArcadeGamepadDevice::compatible_section_descriptor(uint8_t *dest, size_t remaining)
{
    return 0;
}

size_t GHArcadeGamepadDevice::config_descriptor(uint8_t *dest, size_t remaining)
{
    uint8_t desc[] = {TUD_HID_DESCRIPTOR(interface_id, 0, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report_arcade), m_epin, CFG_TUD_HID_EP_BUFSIZE, 1)};
    assert(sizeof(desc) <= remaining);
    memcpy(dest, desc, sizeof(desc));
    return sizeof(desc);
}
size_t GHArcadeGamepadDevice::device_name(uint8_t idx, char *desc) 
{
    if (m_strid != idx) {
        return 0;
    }
    memcpy(desc, str_gha_input, sizeof(str_gha_input));
    return sizeof(str_gha_input);
}

void GHArcadeGamepadDevice::device_descriptor(tusb_desc_device_t *desc)
{
    desc->idVendor = 0x0c70;
    desc->idProduct = 0x0777;
}
const uint8_t *GHArcadeGamepadDevice::report_descriptor()
{
    return desc_hid_report_arcade;
}

uint16_t GHArcadeGamepadDevice::report_desc_len()
{
    return sizeof(desc_hid_report_arcade);
}

void GHArcadeGamepadDevice::set_report(uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize)
{
}

uint16_t GHArcadeGamepadDevice::get_report(uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
{
    return 0;
}