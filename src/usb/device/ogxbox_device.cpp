#include "tusb_option.h"
#include "class/hid/hid.h"
#include "common/tusb_common.h"
#include "device/usbd_pvt.h"
#include "usb/device/ogxbox_device.h"
#include "usb/device/hid_device.h"
#include "usb/usb_descriptors.h"
#include "config.hpp"

const XID_DESCRIPTOR DukeXIDDescriptor = {
    bLength : sizeof(XID_DESCRIPTOR),
    bDescriptorType : 0x42,
    bcdXid : 0x0100,
    bType : 0x01,
    bSubType : 0x02,
    bMaxInputReportSize : sizeof(OGXboxGamepad_Data_t),
    bMaxOutputReportSize : sizeof(OGXboxOutput_Report_t),
    wAlternateProductIds : {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF}
};

const OGXboxGamepadCapabilities_Data_t DukeXIDInputCapabilities = {
    rid : 0x00,
    rsize : sizeof(OGXboxGamepadCapabilities_Data_t),
    dpadUp : 1,
    dpadDown : 1,
    dpadLeft : 1,
    dpadRight : 1,
    start : 1,
    back : 1,
    leftThumbClick : 1,
    rightThumbClick : 1,
    padding : 0,
    a : 0xFF,
    b : 0xFF,
    x : 0xFF,
    y : 0xFF,
    leftShoulder : 0xFF,
    rightShoulder : 0xFF,
    leftTrigger : 0xFF,
    rightTrigger : 0xFF,
    leftStickX : UINT16_MAX,
    leftStickY : UINT16_MAX,
    rightStickX : UINT16_MAX,
    rightStickY : UINT16_MAX,
};

const OGXboxOutput_Report_t DukeXIDVibrationCapabilities = {
    rid : 0x00,
    rsize : sizeof(OGXboxOutput_Report_t),
    left : 0xFFFF,
    right : 0xFFFF
};


OGXboxGamepadDevice::OGXboxGamepadDevice()
{
}
uint16_t OGXboxGamepadDevice::open(tusb_desc_interface_t const *itf_desc, uint16_t max_len)
{

    TU_VERIFY(0x58 == itf_desc->bInterfaceClass, 0);
    uint16_t drv_len = sizeof(tusb_desc_interface_t) +
                       (itf_desc->bNumEndpoints * sizeof(tusb_desc_endpoint_t));

    TU_VERIFY(max_len >= drv_len, 0);

    uint8_t const *p_desc = (uint8_t const *)itf_desc;
    p_desc = tu_desc_next(p_desc);
    TU_ASSERT(usbd_open_edpt_pair(TUD_OPT_RHPORT, p_desc, itf_desc->bNumEndpoints, TUSB_XFER_INTERRUPT, &m_epout, &m_epin), 0);

    // Prepare for output endpoint
    if (m_epout)
    {
        if (!usbd_edpt_xfer(TUD_OPT_RHPORT, m_epout, epout_buf, sizeof(epout_buf)))
        {
            TU_LOG_FAILED();
            TU_BREAKPOINT();
        }
    }
    return drv_len;
}

bool OGXboxGamepadDevice::interrupt_xfer(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
    if (tu_edpt_dir(ep_addr) == TUSB_DIR_IN)
    {
        // Input report
        if (XFER_RESULT_SUCCESS == result)
        {
        }
    }
    else
    {
        // Output report
        if (XFER_RESULT_SUCCESS == result)
        {
            // TODO: rumble
            // return dev->interrupt_received(xferred_bytes);
        }

        // prepare for new transfer
        TU_ASSERT(usbd_edpt_xfer(TUD_OPT_RHPORT, m_epout, epout_buf, 0x40));
    }
    return true;
}
bool OGXboxGamepadDevice::control_transfer(uint8_t stage, tusb_control_request_t const *request)
{
    if (request->bmRequestType_bit.direction == TUSB_DIR_IN)
    {
        if (request->bmRequestType_bit.type == TUSB_REQ_TYPE_VENDOR)
        {
            if (request->bmRequestType_bit.recipient == TUSB_REQ_RCPT_INTERFACE)
            {
                if (request->bRequest == 1 && request->wValue == 0x0100)
                {
                    if (stage == CONTROL_STAGE_SETUP)
                    {
                        tud_control_xfer(TUD_OPT_RHPORT, request, (void *)&DukeXIDInputCapabilities, sizeof(DukeXIDInputCapabilities));
                    }
                    return true;
                }
                if (request->bRequest == 1 && request->wValue == 0x0200)
                {
                    if (stage == CONTROL_STAGE_SETUP)
                    {
                        tud_control_xfer(TUD_OPT_RHPORT, request, (void *)&DukeXIDVibrationCapabilities, sizeof(DukeXIDVibrationCapabilities));
                    }
                    return true;
                }
                if (request->bRequest == 6 && request->wValue == 0x4200)
                {
                    if (stage == CONTROL_STAGE_SETUP)
                    {
                        tud_control_xfer(TUD_OPT_RHPORT, request, (void *)&DukeXIDDescriptor, sizeof(DukeXIDDescriptor));
                    }
                    return true;
                }
            }
        }
    }
    return false;
}

void OGXboxGamepadDevice::initialize()
{
}
void OGXboxGamepadDevice::process(bool full_poll)
{
    if (!tud_ready() || !m_eps_assigned || usbd_edpt_busy(TUD_OPT_RHPORT, m_epin))
        return;
    OGXboxGamepad_Data_t *report = (OGXboxGamepad_Data_t *)epin_buf;
    report->rid = 0;
    report->rsize = sizeof(OGXboxGamepad_Data_t);
    memset(epin_buf, 0, sizeof(epin_buf));
    for (const auto &mapping : mappings)
    {
        mapping->update(full_poll);
        mapping->update_ogxbox(epin_buf);
    }
    if (current_type == GuitarHeroGuitar)
    {
        // convert bitmask slider to actual hid slider
        OGXboxGuitarHeroGuitar_Data_t *reportGh = (OGXboxGuitarHeroGuitar_Data_t *)report;
        reportGh->slider = -((int8_t)((GuitarHeroGuitarAxisMapping::gh5_slider_mapping[reportGh->slider]) ^ 0x80) * -257);
    }

    if (!usbd_edpt_claim(TUD_OPT_RHPORT, m_epin))
    {
        return;
    }
    usbd_edpt_xfer(TUD_OPT_RHPORT, m_epin, epin_buf, sizeof(OGXboxGamepad_Data_t));
}

size_t OGXboxGamepadDevice::compatible_section_descriptor(uint8_t *dest, size_t remaining)
{
}

size_t OGXboxGamepadDevice::config_descriptor(uint8_t *dest, size_t remaining)
{
    if (!m_eps_assigned)
    {
        m_eps_assigned = true;
        m_epin = next_epin();
        m_epout = next_epout();
        usb_instances_by_epnum[m_epin] = usb_instances[interface_id];
        usb_instances_by_epnum[m_epout] = usb_instances[interface_id];
    }
    uint8_t desc[] = {TUD_OGXBOX_GAMEPAD_DESCRIPTOR(interface_id, m_epin, m_epout)};
    assert(sizeof(desc) <= remaining);
    memcpy(dest, desc, sizeof(desc));
    return sizeof(desc);
}

void OGXboxGamepadDevice::device_descriptor(tusb_desc_device_t *desc)
{
    desc->idVendor = 0x045E;
    desc->idProduct = 0x0289;
}
