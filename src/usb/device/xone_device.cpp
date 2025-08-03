#include "tusb_option.h"
// Combine together the implementation of xone and hid as they are baiscally the same
#if (TUSB_OPT_DEVICE_ENABLED && CFG_TUD_XONE)

//--------------------------------------------------------------------+
// INCLUDE
//--------------------------------------------------------------------+
#include "class/hid/hid.h"
#include "common/tusb_common.h"
#include "device/usbd_pvt.h"
#include "usb/device/xone_device.h"

//--------------------------------------------------------------------+
// MACRO CONSTANT TYPEDEF
//--------------------------------------------------------------------+
typedef struct
{
    uint8_t itf_num;
    uint8_t ep_in;
    uint8_t ep_out;

    CFG_TUSB_MEM_ALIGN uint8_t epin_buf[CFG_TUD_XONE_TX_BUFSIZE];
    CFG_TUSB_MEM_ALIGN uint8_t epout_buf[CFG_TUD_XONE_RX_BUFSIZE];
} xoned_interface_t;

CFG_TUSB_MEM_SECTION static xoned_interface_t _xoned_itf[CFG_TUD_XONE];
static volatile bool sending = false;
/*------------- Helpers -------------*/
static inline uint8_t get_index_by_itfnum(uint8_t itf_num)
{
    for (uint8_t i = 0; i < CFG_TUD_XONE; i++)
    {
        if (itf_num == _xoned_itf[i].itf_num)
            return i;
    }

    return 0xFF;
}

//--------------------------------------------------------------------+
// APPLICATION API
//--------------------------------------------------------------------+
bool tud_xone_n_ready(uint8_t itf)
{
    uint8_t const ep_in = _xoned_itf[itf].ep_in;
    return tud_ready() && (ep_in != 0) && !usbd_edpt_busy(TUD_OPT_RHPORT, ep_in);
}

bool tud_xone_n_report(uint8_t itf, void const *report, uint8_t len)
{
    uint8_t const rhport = 0;
    xoned_interface_t *p_xone = &_xoned_itf[itf];

    // claim endpoint
    TU_VERIFY(usbd_edpt_claim(rhport, p_xone->ep_in));

    // If report id = 0, skip ID field
    len = tu_min8(len, CFG_TUD_XONE_TX_BUFSIZE);
    memcpy(p_xone->epin_buf, report, len);
    sending = true;
    return usbd_edpt_xfer(TUD_OPT_RHPORT, p_xone->ep_in, p_xone->epin_buf,
                          len);
}

//--------------------------------------------------------------------+
// USBD-CLASS API
//--------------------------------------------------------------------+
void xoned_init(void)
{
    xoned_reset(TUD_OPT_RHPORT);
}

void xoned_reset(uint8_t rhport)
{
    (void)rhport;
    tu_memclr(_xoned_itf, sizeof(_xoned_itf));
    sending = false;
}

uint16_t xoned_open(uint8_t rhport, tusb_desc_interface_t const *itf_desc,
                    uint16_t max_len)
{
    TU_VERIFY(TUSB_CLASS_VENDOR_SPECIFIC == itf_desc->bInterfaceClass && itf_desc->bInterfaceSubClass == 0x47 &&
                  itf_desc->bInterfaceProtocol == 0xD0,
              0);
    uint16_t drv_len = sizeof(tusb_desc_interface_t) +
                       (itf_desc->bNumEndpoints * sizeof(tusb_desc_endpoint_t));

    TU_VERIFY(max_len >= drv_len, 0);

    // Find available interface
    xoned_interface_t *p_xone = NULL;
    for (uint8_t i = 0; i < CFG_TUD_XONE; i++)
    {
        if (_xoned_itf[i].ep_in == 0 && _xoned_itf[i].ep_out == 0)
        {
            p_xone = &_xoned_itf[i];
            break;
        }
    }
    TU_VERIFY(p_xone, 0);
    uint8_t const *p_desc = (uint8_t const *)itf_desc;
    p_desc = tu_desc_next(p_desc);
    TU_ASSERT(usbd_open_edpt_pair(rhport, p_desc, itf_desc->bNumEndpoints, TUSB_XFER_INTERRUPT, &p_xone->ep_out, &p_xone->ep_in), 0);

    p_xone->itf_num = itf_desc->bInterfaceNumber;

    // Prepare for output endpoint
    if (p_xone->ep_out)
    {
        if (!usbd_edpt_xfer(rhport, p_xone->ep_out, p_xone->epout_buf, sizeof(p_xone->epout_buf)))
        {
            TU_LOG_FAILED();
            TU_BREAKPOINT();
        }
    }
    //------------- Endpoint Descriptor -------------//

    // Config endpoint

    return drv_len;
}

bool xoned_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request) {
  return false;
}

// TODO: we probably should just handle the protocol here.
bool xoned_xfer_cb(uint8_t rhport, uint8_t ep_addr, xfer_result_t result,
                   uint32_t xferred_bytes)
{
    (void)result;

    uint8_t itf = 0;
    xoned_interface_t *p_xone = _xoned_itf;

    for (;; itf++, p_xone++)
    {
        if (itf >= TU_ARRAY_SIZE(_xoned_itf))
            return false;

        if (ep_addr == p_xone->ep_out || ep_addr == p_xone->ep_in)
            break;
    }
    if (ep_addr == p_xone->ep_out)
    {
        // TODO: process the protocol here and specifically parse out things like rumble and leds where necessary
        // tud_xone_set_report_cb(itf, p_xone->epout_buf, xferred_bytes);
        TU_ASSERT(usbd_edpt_xfer(rhport, p_xone->ep_out, p_xone->epout_buf,
                                 0x40));
    }
    else if (ep_addr == p_xone->ep_in)
    {
        sending = false;
    }
    return true;
}

#endif