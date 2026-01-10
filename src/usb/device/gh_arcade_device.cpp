#include "tusb_option.h"
// Driver for emulate a GH Arcade guitar's vendor interface. This simply just gets called by the game for setting the guitars side.
// We don't need to care about the input interfaces.

//--------------------------------------------------------------------+
// INCLUDE
//--------------------------------------------------------------------+
#include "class/hid/hid.h"
#include "common/tusb_common.h"
#include "device/usbd_pvt.h"
#include "usb/device/gh_arcade_device.h"
#include "hid_reports.h"
#include "config.hpp"

static uint8_t const desc_hid_report_arcade[] = {TUD_HID_REPORT_DESC_GUITAR_HERO_ARCADE()};
static std::map<uint8_t, std::shared_ptr<GHArcadeVendorDevice>> ghArcadeDevicesByEpOut;
//--------------------------------------------------------------------+
// USBD-CLASS API
//--------------------------------------------------------------------+
void gh_arcaded_init(void)
{
    gh_arcaded_reset(TUD_OPT_RHPORT);
}

void gh_arcaded_reset(uint8_t rhport)
{
    (void)rhport;
}

uint16_t gh_arcaded_open(uint8_t rhport, tusb_desc_interface_t const *itf_desc,
                         uint16_t max_len)
{
    TU_VERIFY(TUSB_CLASS_VENDOR_SPECIFIC == itf_desc->bInterfaceClass && itf_desc->bInterfaceSubClass == 0x01 && itf_desc->bInterfaceProtocol == 0xFF, 0);
    uint16_t drv_len;
    drv_len = sizeof(tusb_desc_interface_t) +
              (itf_desc->bNumEndpoints * sizeof(tusb_desc_endpoint_t));

    TU_VERIFY(max_len >= drv_len, 0);

    std::shared_ptr<GHArcadeVendorDevice> dev = std::static_pointer_cast<GHArcadeVendorDevice>(usb_instances[itf_desc->bInterfaceNumber]);

    uint8_t const *p_desc = (uint8_t const *)itf_desc;

    p_desc = tu_desc_next(p_desc);
    TU_ASSERT(usbd_open_edpt_pair(rhport, p_desc, itf_desc->bNumEndpoints,
                                  TUSB_XFER_INTERRUPT, &dev->m_epout,
                                  &dev->m_epin1),
              0);
    if (dev->m_epout)
    {
        if (!usbd_edpt_xfer(rhport, dev->m_epout, dev->epout_buf, sizeof(dev->epout_buf)))
        {
            TU_LOG_FAILED();
            TU_BREAKPOINT();
        }
    }
    return drv_len;
}

bool gh_arcaded_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request)
{
    return false;
}

bool gh_arcaded_xfer_cb(uint8_t rhport, uint8_t ep_addr, xfer_result_t result,
                        uint32_t xferred_bytes)
{
    (void)result;

    auto it = ghArcadeDevicesByEpOut.find(ep_addr);
    if (it == ghArcadeDevicesByEpOut.end())
    {
        return false;
    }
    auto dev = it->second;
    if (dev->epout_buf[0] == 0x03)
    {
        // packet sends 0 for left and 1 for right, hid report uses 1 for left and 2 for right
        uint8_t side = dev->epout_buf[1] + 1;
        dev->side = side;
    }
    TU_ASSERT(usbd_edpt_xfer(rhport, dev->m_epout, dev->epout_buf,
                             0x40));
    return true;
}

GHArcadeVendorDevice::GHArcadeVendorDevice()
{
}
void GHArcadeVendorDevice::initialize()
{
}
void GHArcadeVendorDevice::process(bool full_poll)
{
    return;
}

size_t GHArcadeVendorDevice::compatible_section_descriptor(uint8_t *dest, size_t remaining)
{
    return 0;
}

size_t GHArcadeVendorDevice::config_descriptor(uint8_t *dest, size_t remaining)
{
    if (!m_eps_assigned) {
        m_eps_assigned = true;
        m_epin1 = next_epin();
        m_epin2 = next_epin();
        m_epout = next_epin();
    }
    uint8_t desc[] = {TUD_GHARCADE_VENDOR_DESCRIPTOR(m_interface, m_epin1, m_epout, m_epin2, STRID_GHA_LED)};
    assert(sizeof(desc) <= remaining);
    memcpy(dest, desc, sizeof(desc));
    return sizeof(desc);
}

void GHArcadeVendorDevice::device_descriptor(tusb_desc_device_t *desc)
{
    desc->idVendor = 0x0c70;
    desc->idProduct = 0x0777;
}

GHArcadeGamepadDevice::GHArcadeGamepadDevice()
{
}
void GHArcadeGamepadDevice::initialize()
{
}
void GHArcadeGamepadDevice::process(bool full_poll)
{
    return;
}

size_t GHArcadeGamepadDevice::compatible_section_descriptor(uint8_t *dest, size_t remaining)
{
    return 0;
}

size_t GHArcadeGamepadDevice::config_descriptor(uint8_t *dest, size_t remaining)
{
    uint8_t desc[] = {TUD_HID_DESCRIPTOR(m_interface, 0, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report_arcade), m_epin, CFG_TUD_HID_EP_BUFSIZE, 1)};
    assert(sizeof(desc) <= remaining);
    memcpy(dest, desc, sizeof(desc));
    return sizeof(desc);
}

void GHArcadeGamepadDevice::device_descriptor(tusb_desc_device_t *desc)
{
    desc->idVendor = 0x0c70;
    desc->idProduct = 0x0777;
}
const uint8_t * GHArcadeGamepadDevice::report_descriptor()
{
  return desc_hid_report_arcade;
}

uint16_t GHArcadeGamepadDevice::report_desc_len() {
  return sizeof(desc_hid_report_arcade);
}

void GHArcadeGamepadDevice::set_report(uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize)
{
  
}

uint16_t GHArcadeGamepadDevice::get_report(uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
{
  return 0;
}