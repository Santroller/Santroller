#include "tusb_option.h"

//--------------------------------------------------------------------+
// INCLUDE
//--------------------------------------------------------------------+
#include "class/hid/hid.h"
#include "common/tusb_common.h"
#include "device/usbd_pvt.h"
#include "usb/device/xinput_device.h"
#include "usb/device/hid_device.h"
#include "usb/usb_descriptors.h"
#include <xsm3.h>
#include <pico/unique_id.h>
#include "usb/usb_devices.h"
#include "config.hpp"

static const uint8_t xbox_players[] = {
    0, // 0x00	 All off
    0, // 0x01	 All blinking
    1, // 0x02	 1 flashes, then on
    2, // 0x03	 2 flashes, then on
    3, // 0x04	 3 flashes, then on
    4, // 0x05	 4 flashes, then on
    1, // 0x06	 1 on
    2, // 0x07	 2 on
    3, // 0x08	 3 on
    4, // 0x09	 4 on
    0, // 0x0A	 Rotating (e.g. 1-2-4-3)
    0, // 0x0B	 Blinking*
    0, // 0x0C	 Slow blinking*
    0, // 0x0D	 Alternating (e.g. 1+4-2+3), then back to previous*
};
XInputGamepadDevice::XInputGamepadDevice()
{
    lastIntf = 0;
    memset(xinputInterfaces, 0xFF, sizeof(xinputInterfaces));
}
void XInputGamepadDevice::initialize()
{
}

uint16_t XInputGamepadDevice::open(tusb_desc_interface_t const *itf_desc, uint16_t max_len)
{

    TU_VERIFY(TUSB_CLASS_VENDOR_SPECIFIC == itf_desc->bInterfaceClass, 0);
    uint16_t drv_len;
    drv_len = sizeof(tusb_desc_interface_t) +
              (itf_desc->bNumEndpoints * sizeof(tusb_desc_endpoint_t));

    TU_VERIFY(max_len >= drv_len, 0);

    uint8_t const *p_desc = (uint8_t const *)itf_desc;

    if (itf_desc->bInterfaceSubClass == 0x5D &&
        (itf_desc->bInterfaceProtocol == 0x01 ||
         itf_desc->bInterfaceProtocol == 0x03 ||
         itf_desc->bInterfaceProtocol == 0x02))
    {
        // Xinput reserved endpoint
        //-------------- Xinput Descriptor --------------//
        p_desc = tu_desc_next(p_desc);
        XBOX_ID_DESCRIPTOR *x_desc =
            (XBOX_ID_DESCRIPTOR *)p_desc;
        TU_ASSERT(XINPUT_DESC_TYPE_RESERVED == x_desc->bDescriptorType, 0);
        drv_len += x_desc->bLength;
        p_desc = tu_desc_next(p_desc);
        TU_ASSERT(usbd_open_edpt_pair(TUD_OPT_RHPORT, p_desc, itf_desc->bNumEndpoints,
                                      TUSB_XFER_INTERRUPT, &m_epout,
                                      &m_epin),
                  0);
        if (m_epout)
        {
            if (!usbd_edpt_xfer(TUD_OPT_RHPORT, m_epout, epout_buf, sizeof(epout_buf)))
            {
                TU_LOG_FAILED();
                TU_BREAKPOINT();
            }
        }
        for (size_t i = 0; i < sizeof(xinputInterfaces); i++)
        {
            if (xinputInterfaces[i] == 0xFF)
            {
                xinputInterfaces[i] = itf_desc->bInterfaceNumber;
                printf("assigned: %02x, %02x\r\n", i, itf_desc->bInterfaceNumber);
                break;
            }
        }
        return drv_len;
    }
    return 0;
}
void XInputGamepadDevice::process(bool full_poll)
{
    if (!tud_ready() || !m_eps_assigned || usbd_edpt_busy(TUD_OPT_RHPORT, m_epin))
        return;
    memset(epin_buf, 0, sizeof(epin_buf));
    XInputGamepad_Data_t *report = (XInputGamepad_Data_t *)epin_buf;
    report->rid = 0;
    report->rsize = sizeof(XInputGamepad_Data_t);
    for (const auto &mapping : mappings)
    {
        mapping->update(full_poll);
        mapping->update_xinput(epin_buf);
    }
    if (subtype == GuitarHeroGuitar)
    {
        // convert bitmask slider to actual hid slider
        XInputGuitarHeroGuitar_Data_t *reportGh = (XInputGuitarHeroGuitar_Data_t *)report;
        reportGh->slider = -((int8_t)((GuitarHeroGuitarAxisMapping::gh5_slider_mapping[reportGh->slider]) ^ 0x80) * -257);
    }

    if (!usbd_edpt_claim(TUD_OPT_RHPORT, m_epin))
    {
        return;
    }

    usbd_edpt_xfer(TUD_OPT_RHPORT, m_epin, epin_buf, sizeof(XInputGamepad_Data_t));
}

bool XInputGamepadDevice::interrupt_xfer(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
    if (tu_edpt_dir(ep_addr) == TUSB_DIR_IN)
    {
        return true;
    }

    if (XFER_RESULT_SUCCESS == result)
    {
        XInputLEDReport_t const *ledReport = (XInputLEDReport_t const *)epout_buf;
        XInputRumbleReport_t const *rumbleReport = (XInputRumbleReport_t const *)epout_buf;
        if (ledReport->rid == XBOX_LED_ID)
        {
            uint8_t player = xbox_players[ledReport->led];
            if (player)
            {
                player_led = player;
            }
        }
        else if (rumbleReport->rid == XBOX_RUMBLE_ID)
        {
            if (subtype == DjHeroTurntable)
            {
                euphoria_led = rumbleReport->leftRumble;
            }
            else if (subtype == StageKit)
            {
                stagekit_command = rumbleReport->rightRumble;
                stagekit_param = rumbleReport->leftRumble;
            }
            else
            {
                rumble_left = rumbleReport->leftRumble;
                rumble_right = rumbleReport->rightRumble;
            }
        }
        TU_ASSERT(usbd_edpt_xfer(TUD_OPT_RHPORT, m_epout, epout_buf, 0x20));
    }
    return true;
}
bool XInputGamepadDevice::control_transfer(uint8_t stage, tusb_control_request_t const *request)
{
    if (request->bmRequestType_bit.direction == TUSB_DIR_IN)
    {
        if (request->bmRequestType_bit.type == TUSB_REQ_TYPE_VENDOR)
        {
            if (request->bmRequestType_bit.recipient == TUSB_REQ_RCPT_INTERFACE && request->bRequest == HID_REQ_CONTROL_GET_REPORT)
            {
                if (request->wValue == VIBRATION_CAPABILITIES_WVALUE)
                {
                    if (stage == CONTROL_STAGE_SETUP)
                    {
                        printf("vibr\r\n");
                        XInputVibrationCapabilities_t XInputVibrationCapabilities = {
                            rid : 0x00,
                            rsize : sizeof(XInputVibrationCapabilities_t),
                            padding : 0x00,
                            left_motor : 0xFF,
                            right_motor : 0xFF,
                            padding_2 : {0x00, 0x00, 0x00}
                        };
                        tud_control_xfer(TUD_OPT_RHPORT, request, (void *)&XInputVibrationCapabilities, sizeof(XInputVibrationCapabilities_t));
                    }
                    return true;
                }
                else if (request->wValue == INPUT_CAPABILITIES_WVALUE)
                {
                    if (stage == CONTROL_STAGE_SETUP)
                    {
                        printf("caps\r\n");
                        XInputInputCapabilities_t XInputInputCapabilities = {
                            rid : 0x00,
                            rsize : sizeof(XInputInputCapabilities_t),
                            buttons : 0xf73f,
                            leftTrigger : 0xff,
                            rightTrigger : 0xff,
                            leftThumbX : USB_VID,
                            leftThumbY : USB_PID,
                            rightThumbX : (USB_VERSION_BCD(subtype, 0, 0)),
                            rightThumbY : 0xffc0,
                            reserved : {0x00, 0x00, 0x00, 0x00},
                            flags : XINPUT_FLAGS_FORCE_FEEDBACK
                        };
                        switch (subtype)
                        {
                        case LiveGuitar:
                            XInputInputCapabilities.flags = XINPUT_FLAGS_NO_NAV;
                            break;
                        case ProGuitarMustang:
                            XInputInputCapabilities.flags = XINPUT_FLAGS_NONE;
                            XInputInputCapabilities.leftThumbX = HARMONIX_VID;
                            XInputInputCapabilities.leftThumbY = XBOX_360_MUSTANG_PID;
                            break;
                        case ProGuitarSquire:
                            XInputInputCapabilities.flags = XINPUT_FLAGS_NONE;
                            XInputInputCapabilities.leftThumbX = HARMONIX_VID;
                            XInputInputCapabilities.leftThumbY = XBOX_360_SQUIRE_PID;
                            break;
                        case GuitarHeroDrums:
                            XInputInputCapabilities.flags = XINPUT_FLAGS_NONE;
                            break;
                        case DjHeroTurntable:
                            XInputInputCapabilities.flags = XINPUT_FLAGS_NONE;
                            break;
                        default:
                            break;
                        }
                        XInputInputCapabilities.rightThumbX = USB_VERSION_BCD(subtype, 0, 0);
                        tud_control_xfer(TUD_OPT_RHPORT, request, (void *)&XInputInputCapabilities, sizeof(XInputInputCapabilities_t));
                    }
                    return true;
                }
            }
        }
    }
    return false;
}

size_t XInputGamepadDevice::compatible_section_descriptor(uint8_t *dest, size_t remaining)
{
    OS_COMPATIBLE_SECTION section = {
        FirstInterfaceNumber : interface_id,
        Reserved : 0x01,
        CompatibleID : "XUSB10",
        SubCompatibleID : {0},
        Reserved2 : {0}
    };
    assert(sizeof(section) <= remaining);
    memcpy(dest, &section, sizeof(section));
    return sizeof(section);
}

size_t XInputGamepadDevice::config_descriptor(uint8_t *dest, size_t remaining)
{
    if (!m_eps_assigned)
    {
        m_eps_assigned = true;
        m_epin = next_epin();
        m_epout = next_epout();
        usb_instances_by_epnum[m_epin] = usb_instances[interface_id];
        usb_instances_by_epnum[m_epout] = usb_instances[interface_id];
    }
    uint8_t desc[] = {TUD_XINPUT_GAMEPAD_DESCRIPTOR(interface_id, m_epin, m_epout, get_xinput_subtype(subtype))};
    assert(sizeof(desc) <= remaining);
    memcpy(dest, desc, sizeof(desc));
    return sizeof(desc);
}

void XInputGamepadDevice::device_descriptor(tusb_desc_device_t *desc)
{
}

XInputSecurityDevice::XInputSecurityDevice()
{
}
void XInputSecurityDevice::initialize()
{
}
void XInputSecurityDevice::process(bool full_poll)
{
}

uint16_t XInputSecurityDevice::open(tusb_desc_interface_t const *itf_desc, uint16_t max_len)
{

    TU_VERIFY(TUSB_CLASS_VENDOR_SPECIFIC == itf_desc->bInterfaceClass, 0);
    uint16_t drv_len;
    drv_len = sizeof(tusb_desc_interface_t) +
              (itf_desc->bNumEndpoints * sizeof(tusb_desc_endpoint_t));

    TU_VERIFY(max_len >= drv_len, 0);

    uint8_t const *p_desc = (uint8_t const *)itf_desc;

    if (itf_desc->bInterfaceSubClass == 0xfD &&
        itf_desc->bInterfaceProtocol == 0x13)
    {
        // xinput security interface
        p_desc = tu_desc_next(p_desc);
        XBOX_SECURITY_DESCRIPTOR *x_desc =
            (XBOX_SECURITY_DESCRIPTOR *)p_desc;
        TU_ASSERT(XINPUT_SECURITY_DESC_TYPE_RESERVED == x_desc->bDescriptorType, 0);
        drv_len += x_desc->bLength;
        return drv_len;
    }
    return 0;
}

size_t XInputSecurityDevice::compatible_section_descriptor(uint8_t *dest, size_t remaining)
{
    OS_COMPATIBLE_SECTION section = {
        FirstInterfaceNumber : interface_id,
        Reserved : 0x01,
        CompatibleID : "XUSB10",
        SubCompatibleID : {0},
        Reserved2 : {0}
    };
    assert(sizeof(section) <= remaining);
    memcpy(dest, &section, sizeof(section));
    return sizeof(section);
}

size_t XInputSecurityDevice::config_descriptor(uint8_t *dest, size_t remaining)
{
    uint8_t desc[] = {TUD_XINPUT_SECURITY_DESCRIPTOR(interface_id, STRID_XSM3)};
    assert(sizeof(desc) <= remaining);
    memcpy(dest, desc, sizeof(desc));
    return sizeof(desc);
}

void XInputSecurityDevice::device_descriptor(tusb_desc_device_t *desc)
{
}

bool XInputSecurityDevice::interrupt_xfer(uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
    return false;
}
bool XInputSecurityDevice::control_transfer(uint8_t stage, tusb_control_request_t const *request)
{
    static uint8_t buf[0x22];
    if (request->bmRequestType_bit.direction == TUSB_DIR_IN)
    {
        switch (request->bRequest)
        {
        case 0x81:
            if (stage == CONTROL_STAGE_SETUP)
            {
                uint8_t serial[0x0C];
                pico_get_unique_board_id_string((char *)serial, sizeof(serial));
                serial[0] = to_us_since_boot(get_absolute_time());
                xsm3_set_vid_pid(serial, USB_VID, USB_PID);
                xsm3_initialise_state();
                xsm3_set_identification_data(xsm3_id_data_ms_controller);
                tud_control_xfer(TUD_OPT_RHPORT, request, xsm3_id_data_ms_controller, sizeof(xsm3_id_data_ms_controller));
            }
            newMode = ModeXbox360;
            return true;
        case 0x82:
            if (stage == CONTROL_STAGE_SETUP)
            {
                tud_control_xfer(TUD_OPT_RHPORT, request, buf, request->wLength);
                xsm3_do_challenge_init(buf);
            }
            return true;
        case 0x87:
            if (stage == CONTROL_STAGE_SETUP)
            {
                tud_control_xfer(TUD_OPT_RHPORT, request, buf, request->wLength);
                xsm3_do_challenge_verify(buf);
            }
            return true;
        case 0x83:
            if (stage == CONTROL_STAGE_SETUP)
            {
                tud_control_xfer(TUD_OPT_RHPORT, request, xsm3_challenge_response, sizeof(xsm3_challenge_response));
            }
            return true;
        case 0x86:
            if (stage == CONTROL_STAGE_SETUP)
            {
                short state = 2; // 1 = in-progress, 2 = complete
                tud_control_xfer(TUD_OPT_RHPORT, request, &state, sizeof(state));
            }
            return true;
        }
    }
    else
    {
        if (stage == CONTROL_STAGE_DATA || stage == CONTROL_STAGE_SETUP)
        {
            switch (request->bRequest)
            {
            case 0x82:
                if (stage == CONTROL_STAGE_DATA)
                {
                    xsm3_do_challenge_init(buf);
                }
                else
                {
                    tud_control_xfer(TUD_OPT_RHPORT, request, buf, request->wLength);
                }
                return true;
            case 0x87:
                if (stage == CONTROL_STAGE_DATA)
                {
                    xsm3_do_challenge_verify(buf);
                }
                else
                {
                    tud_control_xfer(TUD_OPT_RHPORT, request, buf, request->wLength);
                }
                return true;
            case 0x84:
                if (stage == CONTROL_STAGE_SETUP)
                {
                    tud_control_xfer(TUD_OPT_RHPORT, request, buf, request->wLength);
                }
                return true;
            }
        }
    }

    return false;
}

uint8_t XInputGamepadDevice::xinputInterfaces[] = {};
uint8_t XInputGamepadDevice::lastIntf = 0;