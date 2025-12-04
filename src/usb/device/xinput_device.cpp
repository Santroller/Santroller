#include "tusb_option.h"
// Combine together the implementation of xinput and hid as they are baiscally the same
#if (TUSB_OPT_DEVICE_ENABLED && CFG_TUD_XINPUT)

//--------------------------------------------------------------------+
// INCLUDE
//--------------------------------------------------------------------+
#include "class/hid/hid.h"
#include "common/tusb_common.h"
#include "device/usbd_pvt.h"
#include "usb/device/xinput_device.h"
#include "usb/usb_descriptors.h"
#include <xsm3.h>
#include <pico/unique_id.h>
#include "usb/usb_devices.h"
#include "config.hpp"

//--------------------------------------------------------------------+
// MACRO CONSTANT TYPEDEF
//--------------------------------------------------------------------+
typedef struct
{
    uint8_t itf_num;
    uint8_t ep_in;
    uint8_t ep_out;
    uint8_t subtype;

    CFG_TUSB_MEM_ALIGN uint8_t epin_buf[CFG_TUD_XINPUT_TX_BUFSIZE];
    CFG_TUSB_MEM_ALIGN uint8_t epout_buf[CFG_TUD_XINPUT_RX_BUFSIZE];
} xinputd_interface_t;

uint8_t xbox_players[] = {
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

const XInputVibrationCapabilities_t XInputVibrationCapabilities = {
    rid : 0x00,
    rsize : sizeof(XInputVibrationCapabilities_t),
    padding : 0x00,
    left_motor : 0xFF,
    right_motor : 0xFF,
    padding_2 : {0x00, 0x00, 0x00}
};
XInputInputCapabilities_t XInputInputCapabilities = {
    rid : 0x00,
    rsize : sizeof(XInputInputCapabilities_t),
    buttons : 0xf73f,
    leftTrigger : 0xff,
    rightTrigger : 0xff,
    leftThumbX : USB_VID,
    leftThumbY : USB_PID,
    rightThumbX : 0,
    rightThumbY : 0xffc0,
    reserved : {0x00, 0x00, 0x00, 0x00},
    flags : XINPUT_FLAGS_FORCE_FEEDBACK
};

static const OS_COMPATIBLE_ID_DESCRIPTOR DevCompatIDs = {
    TotalLength : sizeof(OS_COMPATIBLE_ID_DESCRIPTOR),
    Version : 0x0100,
    Index : DESC_EXTENDED_COMPATIBLE_ID_DESCRIPTOR,
    TotalSections : 1,
    Reserved : {0},
    CompatID : {
        {
            FirstInterfaceNumber : 0,
            Reserved : 0x01,
            CompatibleID : "XUSB10",
            SubCompatibleID : {0},
            Reserved2 : {0}
        }}
};
CFG_TUSB_MEM_SECTION static xinputd_interface_t _xinputd_itf[CFG_TUD_XINPUT];
static volatile bool sending = false;
/*------------- Helpers -------------*/
static inline uint8_t get_index_by_itfnum(uint8_t itf_num)
{
    for (uint8_t i = 0; i < CFG_TUD_XINPUT; i++)
    {
        if (itf_num == _xinputd_itf[i].itf_num)
            return i;
    }

    return 0xFF;
}

//--------------------------------------------------------------------+
// APPLICATION API
//--------------------------------------------------------------------+
bool tud_xinput_n_ready(uint8_t itf)
{
    uint8_t const ep_in = _xinputd_itf[itf].ep_in;
    return tud_ready() && (ep_in != 0) && !usbd_edpt_busy(TUD_OPT_RHPORT, ep_in);
}

bool tud_xinput_n_report(uint8_t itf, void const *report,
                         uint8_t len)
{
    uint8_t const rhport = 0;
    xinputd_interface_t *p_xinput = &_xinputd_itf[itf];

    // claim endpoint
    TU_VERIFY(usbd_edpt_claim(rhport, p_xinput->ep_in));

    // If report id = 0, skip ID field
    len = tu_min8(len, CFG_TUD_XINPUT_TX_BUFSIZE);
    memcpy(p_xinput->epin_buf, report, len);
    sending = true;
    return usbd_edpt_xfer(TUD_OPT_RHPORT, p_xinput->ep_in, p_xinput->epin_buf,
                          len);
}

//--------------------------------------------------------------------+
// USBD-CLASS API
//--------------------------------------------------------------------+
void xinputd_init(void)
{
    xinputd_reset(TUD_OPT_RHPORT);
}

void xinputd_reset(uint8_t rhport)
{
    (void)rhport;
    tu_memclr(_xinputd_itf, sizeof(_xinputd_itf));
    for (uint8_t i = 0; i < CFG_TUD_XINPUT; i++)
    {
        _xinputd_itf[i].itf_num = 0xFF;
    }
    sending = false;
}

uint16_t xinputd_open(uint8_t rhport, tusb_desc_interface_t const *itf_desc,
                      uint16_t max_len)
{
    TU_VERIFY(TUSB_CLASS_VENDOR_SPECIFIC == itf_desc->bInterfaceClass, 0);
    uint16_t drv_len;
    drv_len = sizeof(tusb_desc_interface_t) +
              (itf_desc->bNumEndpoints * sizeof(tusb_desc_endpoint_t));

    TU_VERIFY(max_len >= drv_len, 0);

    // Find available interface
    xinputd_interface_t *p_xinput = NULL;
    for (uint8_t i = 0; i < CFG_TUD_XINPUT; i++)
    {
        if (_xinputd_itf[i].ep_in == 0 && _xinputd_itf[i].ep_out == 0)
        {
            p_xinput = &_xinputd_itf[i];
            break;
        }
    }
    TU_VERIFY(p_xinput, 0);
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
        p_xinput->subtype = x_desc->subtype;
        TU_ASSERT(XINPUT_DESC_TYPE_RESERVED == x_desc->bDescriptorType, 0);
        drv_len += x_desc->bLength;
        p_desc = tu_desc_next(p_desc);
        TU_ASSERT(usbd_open_edpt_pair(rhport, p_desc, itf_desc->bNumEndpoints,
                                      TUSB_XFER_INTERRUPT, &p_xinput->ep_out,
                                      &p_xinput->ep_in),
                  0);

        p_xinput->itf_num = itf_desc->bInterfaceNumber;
        if (p_xinput->ep_out)
        {
            if (!usbd_edpt_xfer(rhport, p_xinput->ep_out, p_xinput->epout_buf, sizeof(p_xinput->epout_buf)))
            {
                TU_LOG_FAILED();
                TU_BREAKPOINT();
            }
        }
        return drv_len;
    }
    else if (itf_desc->bInterfaceSubClass == 0xfD &&
             itf_desc->bInterfaceProtocol == 0x13)
    {
        // xinput security interface
        p_desc = tu_desc_next(p_desc);
        XBOX_SECURITY_DESCRIPTOR *x_desc =
            (XBOX_SECURITY_DESCRIPTOR *)p_desc;
        TU_ASSERT(XINPUT_SECURITY_DESC_TYPE_RESERVED == x_desc->bDescriptorType, 0);
        drv_len += x_desc->bLength;
        p_xinput->itf_num = itf_desc->bInterfaceNumber;
        p_xinput->subtype = 0;
        return drv_len;
    }
    return 0;
}
static uint8_t lastIntf = 0;
bool xinputd_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request)
{
    // TODO: how do we differenciate this for multiple xinput interfaces at once
    xinputd_interface_t *p_xinput = _xinputd_itf;
    // if (request->bmRequestType_bit.recipient == TUSB_REQ_RCPT_INTERFACE)
    // {
    //     for (;; itf++, p_xinput++)
    //     {
    //         if (itf >= TU_ARRAY_SIZE(_xinputd_itf))
    //             return false;

    //         if ((request->wIndex & 0xff) == p_xinput->itf_num)
    //             break;
    //     }
    // }
    static uint8_t buf[0x22];
    if (request->bmRequestType_bit.direction == TUSB_DIR_IN)
    {
        if (request->bmRequestType_bit.type == TUSB_REQ_TYPE_VENDOR && request->bRequest == HID_REQ_CONTROL_GET_REPORT)
        {
            if (request->bmRequestType_bit.recipient == TUSB_REQ_RCPT_INTERFACE)
            {
                if (request->wValue == VIBRATION_CAPABILITIES_WVALUE)
                {
                    if (stage == CONTROL_STAGE_SETUP)
                    {
                        tud_control_xfer(rhport, request, (void *)&XInputVibrationCapabilities, sizeof(XInputVibrationCapabilities_t));
                    }
                    return true;
                }
                else if (request->wValue == INPUT_CAPABILITIES_WVALUE)
                {
                    if (stage == CONTROL_STAGE_SETUP)
                    {
                        // TODO: get this somehow
                        SubType SubType = GuitarHeroGuitar;
                        XInputInputCapabilities.flags = XINPUT_FLAGS_FORCE_FEEDBACK;
                        switch (SubType)
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
                        XInputInputCapabilities.rightThumbX = USB_VERSION_BCD(SubType, 0, 0);
                        tud_control_xfer(rhport, request, (void *)&XInputInputCapabilities, sizeof(XInputInputCapabilities_t));
                    }
                    return true;
                }
            }
            else if (request->bmRequestType_bit.recipient == TUSB_REQ_RCPT_DEVICE)
            {
                if (request->bRequest == HID_REQ_CONTROL_GET_REPORT && request->wIndex == 0x0000 && request->wValue == SERIAL_NUMBER_WVALUE)
                {
                    if (stage == CONTROL_STAGE_SETUP)
                    {
                        uint32_t serial = to_us_since_boot(get_absolute_time());
                        tud_control_xfer(rhport, request, &serial, sizeof(serial));
                    }
                    return true;
                }
                else if (request->bRequest == REQ_GET_OS_FEATURE_DESCRIPTOR && request->wIndex == DESC_EXTENDED_COMPATIBLE_ID_DESCRIPTOR)
                {
                    if (stage == CONTROL_STAGE_SETUP)
                    {
                        tud_control_xfer(rhport, request, (void *)&DevCompatIDs, sizeof(OS_COMPATIBLE_ID_DESCRIPTOR));
                    }
                    return true;
                }
            }
        }
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
                tud_control_xfer(rhport, request, xsm3_id_data_ms_controller, sizeof(xsm3_id_data_ms_controller));
            }
            newMode = ConsoleMode::Xbox360;
            return true;
        case 0x82:
            if (stage == CONTROL_STAGE_SETUP)
            {
                tud_control_xfer(rhport, request, buf, request->wLength);
                xsm3_do_challenge_init(buf);
            }
            return true;
        case 0x87:
            if (stage == CONTROL_STAGE_SETUP)
            {
                tud_control_xfer(rhport, request, buf, request->wLength);
                xsm3_do_challenge_verify(buf);
            }
            return true;
        case 0x83:
            if (stage == CONTROL_STAGE_SETUP)
            {
                tud_control_xfer(rhport, request, xsm3_challenge_response, sizeof(xsm3_challenge_response));
                return true;
            }
        case 0x86:
            if (stage == CONTROL_STAGE_SETUP)
            {
                short state = 2; // 1 = in-progress, 2 = complete
                tud_control_xfer(rhport, request, &state, sizeof(state));
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
                    tud_control_xfer(rhport, request, buf, request->wLength);
                }
                return true;
            case 0x87:
                if (stage == CONTROL_STAGE_DATA)
                {
                    xsm3_do_challenge_verify(buf);
                }
                else
                {
                    tud_control_xfer(rhport, request, buf, request->wLength);
                }
                return true;
            case 0x84:
                if (stage == CONTROL_STAGE_SETUP)
                {
                    tud_control_xfer(rhport, request, buf, request->wLength);
                }
                return true;
            }
        }
    }

    return false;
}

bool xinputd_xfer_cb(uint8_t rhport, uint8_t ep_addr, xfer_result_t result,
                     uint32_t xferred_bytes)
{
    (void)result;

    uint8_t itf = 0;
    xinputd_interface_t *p_xinput = _xinputd_itf;

    for (;; itf++, p_xinput++)
    {
        if (itf >= TU_ARRAY_SIZE(_xinputd_itf))
            return false;

        if (ep_addr == p_xinput->ep_out || ep_addr == p_xinput->ep_in)
            break;
    }
    if (ep_addr == p_xinput->ep_out)
    {
        XInputLEDReport_t const *ledReport = (XInputLEDReport_t const *)p_xinput->epout_buf;
        XInputRumbleReport_t const *rumbleReport = (XInputRumbleReport_t const *)p_xinput->epout_buf;
        if (ledReport->rid == XBOX_LED_ID)
        {
            uint8_t player = xbox_players[ledReport->led];
            if (player)
            {
                tud_set_player_led_cb(player);
            }
        }
        else if (rumbleReport->rid == XBOX_RUMBLE_ID)
        {
            if (p_xinput->subtype == XINPUT_TURNTABLE)
            {

                tud_set_euphoria_led_cb(rumbleReport->leftRumble);
            }
            else if (p_xinput->subtype == XINPUT_STAGE_KIT)
            {

                tud_set_stage_kit_cb(rumbleReport->rightRumble, rumbleReport->leftRumble);
            }
            else
            {

                tud_set_rumble_cb(rumbleReport->leftRumble, rumbleReport->rightRumble);
            }
        }
        TU_ASSERT(usbd_edpt_xfer(rhport, p_xinput->ep_out, p_xinput->epout_buf,
                                 0x20));
    }
    else if (ep_addr == p_xinput->ep_in)
    {
        sending = false;
    }
    return true;
}

#endif