#include "tusb_option.h"
// Combine together the implementation of ogxbox and hid as they are baiscally the same
#if (TUSB_OPT_DEVICE_ENABLED && CFG_TUD_OGXBOX)

//--------------------------------------------------------------------+
// INCLUDE
//--------------------------------------------------------------------+
#include "class/hid/hid.h"
#include "common/tusb_common.h"
#include "device/usbd_pvt.h"
#include "usb/device/ogxbox_device.h"

//--------------------------------------------------------------------+
// MACRO CONSTANT TYPEDEF
//--------------------------------------------------------------------+
typedef struct
{
    uint8_t itf_num;
    uint8_t ep_in;
    uint8_t ep_out;

    CFG_TUSB_MEM_ALIGN uint8_t epin_buf[CFG_TUD_OGXBOX_TX_BUFSIZE];
    CFG_TUSB_MEM_ALIGN uint8_t epout_buf[CFG_TUD_OGXBOX_RX_BUFSIZE];
} ogxboxd_interface_t;


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

CFG_TUSB_MEM_SECTION static ogxboxd_interface_t _ogxboxd_itf[CFG_TUD_OGXBOX];
static volatile bool sending = false;
/*------------- Helpers -------------*/
static inline uint8_t get_index_by_itfnum(uint8_t itf_num)
{
    for (uint8_t i = 0; i < CFG_TUD_OGXBOX; i++)
    {
        if (itf_num == _ogxboxd_itf[i].itf_num)
            return i;
    }

    return 0xFF;
}

//--------------------------------------------------------------------+
// APPLICATION API
//--------------------------------------------------------------------+
bool tud_ogxbox_n_ready(uint8_t itf)
{
    uint8_t const ep_in = _ogxboxd_itf[itf].ep_in;
    return tud_ready() && (ep_in != 0) && !usbd_edpt_busy(TUD_OPT_RHPORT, ep_in);
}

bool tud_ogxbox_n_report(uint8_t itf, void const *report, uint8_t len)
{
    uint8_t const rhport = 0;
    ogxboxd_interface_t *p_ogxbox = &_ogxboxd_itf[itf];

    // claim endpoint
    TU_VERIFY(usbd_edpt_claim(rhport, p_ogxbox->ep_in));

    // If report id = 0, skip ID field
    len = tu_min8(len, CFG_TUD_OGXBOX_TX_BUFSIZE);
    memcpy(p_ogxbox->epin_buf, report, len);
    sending = true;
    return usbd_edpt_xfer(TUD_OPT_RHPORT, p_ogxbox->ep_in, p_ogxbox->epin_buf,
                          len);
}

//--------------------------------------------------------------------+
// USBD-CLASS API
//--------------------------------------------------------------------+
void ogxboxd_init(void)
{
    ogxboxd_reset(TUD_OPT_RHPORT);
}

void ogxboxd_reset(uint8_t rhport)
{
    (void)rhport;
    tu_memclr(_ogxboxd_itf, sizeof(_ogxboxd_itf));
    sending = false;
}

uint16_t ogxboxd_open(uint8_t rhport, tusb_desc_interface_t const *itf_desc,
                      uint16_t max_len)
{
    TU_VERIFY(0x58 == itf_desc->bInterfaceClass, 0);
    uint16_t drv_len = sizeof(tusb_desc_interface_t) +
                       (itf_desc->bNumEndpoints * sizeof(tusb_desc_endpoint_t));

    TU_VERIFY(max_len >= drv_len, 0);

    // Find available interface
    ogxboxd_interface_t *p_ogxbox = NULL;
    for (uint8_t i = 0; i < CFG_TUD_OGXBOX; i++)
    {
        if (_ogxboxd_itf[i].ep_in == 0 && _ogxboxd_itf[i].ep_out == 0)
        {
            p_ogxbox = &_ogxboxd_itf[i];
            break;
        }
    }
    TU_VERIFY(p_ogxbox, 0);
    uint8_t const *p_desc = (uint8_t const *)itf_desc;
    p_desc = tu_desc_next(p_desc);
    TU_ASSERT(usbd_open_edpt_pair(rhport, p_desc, itf_desc->bNumEndpoints, TUSB_XFER_INTERRUPT, &p_ogxbox->ep_out, &p_ogxbox->ep_in), 0);

    p_ogxbox->itf_num = itf_desc->bInterfaceNumber;

    // Prepare for output endpoint
    if (p_ogxbox->ep_out)
    {
        if (!usbd_edpt_xfer(rhport, p_ogxbox->ep_out, p_ogxbox->epout_buf, sizeof(p_ogxbox->epout_buf)))
        {
            TU_LOG_FAILED();
            TU_BREAKPOINT();
        }
    }
    //------------- Endpoint Descriptor -------------//

    // Config endpoint

    return drv_len;
}

bool ogxboxd_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request)
{
    if (request->bmRequestType_bit.direction == TUSB_DIR_IN)
    {
        if (stage == CONTROL_STAGE_SETUP)
        {
            if (request->bmRequestType_bit.type == TUSB_REQ_TYPE_VENDOR)
            {
                if (request->bmRequestType_bit.recipient == TUSB_REQ_RCPT_INTERFACE)
                {
                    if (request->bRequest == 6 && request->wValue == 0x4200)
                    {
                        tud_control_xfer(rhport, request, (void *)&DukeXIDDescriptor, sizeof(DukeXIDDescriptor));
                        return true;
                    }
                    if (request->bRequest == 1 && request->wValue == 0x0100)
                    {
                        tud_control_xfer(rhport, request, (void *)&DukeXIDInputCapabilities, sizeof(DukeXIDInputCapabilities));
                        return true;
                    }
                    if (request->bRequest == 1 && request->wValue == 0x0200)
                    {
                        tud_control_xfer(rhport, request, (void *)&DukeXIDVibrationCapabilities, sizeof(DukeXIDVibrationCapabilities));
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

bool ogxboxd_xfer_cb(uint8_t rhport, uint8_t ep_addr, xfer_result_t result,
                     uint32_t xferred_bytes)
{
    (void)result;

    uint8_t itf = 0;
    ogxboxd_interface_t *p_ogxbox = _ogxboxd_itf;

    for (;; itf++, p_ogxbox++)
    {
        if (itf >= TU_ARRAY_SIZE(_ogxboxd_itf))
            return false;

        if (ep_addr == p_ogxbox->ep_out || ep_addr == p_ogxbox->ep_in)
            break;
    }
    if (ep_addr == p_ogxbox->ep_out)
    {
        // tud_ogxbox_set_report_cb(itf, p_ogxbox->epout_buf, xferred_bytes);
        // TODO: parse the report here and call rumble and led funcs
        TU_ASSERT(usbd_edpt_xfer(rhport, p_ogxbox->ep_out, p_ogxbox->epout_buf,
                                 0x40));
    }
    else if (ep_addr == p_ogxbox->ep_in)
    {
        sending = false;
    }
    return true;
}

#endif