#include "tusb_option.h"
// Driver for emulate a GH Arcade guitar's vendor interface. This simply just gets called by the game for setting the guitars side.
// We don't need to care about the input interfaces.
#if (TUSB_OPT_DEVICE_ENABLED && CFG_TUD_GH_ARCADE)

//--------------------------------------------------------------------+
// INCLUDE
//--------------------------------------------------------------------+
#include "class/hid/hid.h"
#include "common/tusb_common.h"
#include "device/usbd_pvt.h"
#include "usb/device/gh_arcade_device.h"

//--------------------------------------------------------------------+
// MACRO CONSTANT TYPEDEF
//--------------------------------------------------------------------+
typedef struct
{
    uint8_t itf_num;
    uint8_t ep_out;
    uint8_t ep_in;

    CFG_TUSB_MEM_ALIGN uint8_t epout_buf[CFG_TUD_GH_ARCADE_RX_BUFSIZE];
} gh_arcaded_interface_t;

CFG_TUSB_MEM_SECTION static gh_arcaded_interface_t _gh_arcaded_itf[CFG_TUD_GH_ARCADE];
static volatile bool sending = false;
/*------------- Helpers -------------*/
static inline uint8_t get_index_by_itfnum(uint8_t itf_num)
{
    for (uint8_t i = 0; i < CFG_TUD_GH_ARCADE; i++)
    {
        if (itf_num == _gh_arcaded_itf[i].itf_num)
            return i;
    }

    return 0xFF;
}

//--------------------------------------------------------------------+
// APPLICATION API
//--------------------------------------------------------------------+
bool tud_gh_arcade_n_ready(uint8_t itf)
{
    uint8_t const ep_in = _gh_arcaded_itf[itf].ep_in;
    return tud_ready() && (ep_in != 0) && !usbd_edpt_busy(TUD_OPT_RHPORT, ep_in);
}

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
    tu_memclr(_gh_arcaded_itf, sizeof(_gh_arcaded_itf));
    sending = false;
}

uint16_t gh_arcaded_open(uint8_t rhport, tusb_desc_interface_t const *itf_desc,
                         uint16_t max_len)
{
    TU_VERIFY(TUSB_CLASS_VENDOR_SPECIFIC == itf_desc->bInterfaceClass && itf_desc->bInterfaceSubClass == 0x01 && itf_desc->bInterfaceProtocol == 0xFF, 0);
    uint16_t drv_len;
    drv_len = sizeof(tusb_desc_interface_t) +
              (itf_desc->bNumEndpoints * sizeof(tusb_desc_endpoint_t));

    TU_VERIFY(max_len >= drv_len, 0);

    // Find available interface
    gh_arcaded_interface_t *p_gh_arcade = NULL;
    for (uint8_t i = 0; i < CFG_TUD_GH_ARCADE; i++)
    {
        if (_gh_arcaded_itf[i].ep_in == 0 && _gh_arcaded_itf[i].ep_out == 0)
        {
            p_gh_arcade = &_gh_arcaded_itf[i];
            break;
        }
    }
    TU_VERIFY(p_gh_arcade, 0);
    uint8_t const *p_desc = (uint8_t const *)itf_desc;

    p_desc = tu_desc_next(p_desc);
    TU_ASSERT(usbd_open_edpt_pair(rhport, p_desc, itf_desc->bNumEndpoints,
                                  TUSB_XFER_INTERRUPT, &p_gh_arcade->ep_out,
                                  &p_gh_arcade->ep_in),
              0);
    if (p_gh_arcade->ep_out)
    {
        if (!usbd_edpt_xfer(rhport, p_gh_arcade->ep_out, p_gh_arcade->epout_buf, sizeof(p_gh_arcade->epout_buf)))
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

    uint8_t itf = 0;
    gh_arcaded_interface_t *p_gh_arcade = _gh_arcaded_itf;

    for (;; itf++, p_gh_arcade++)
    {
        if (itf >= TU_ARRAY_SIZE(_gh_arcaded_itf))
            return false;

        if (ep_addr == p_gh_arcade->ep_out || ep_addr == p_gh_arcade->ep_in)
            break;
    }
    if (ep_addr == p_gh_arcade->ep_out)
    {
        if (p_gh_arcade->epout_buf[0] == 0x03)
        {
            // packet sends 0 for left and 1 for right, hid report uses 1 for left and 2 for right
            uint8_t side = p_gh_arcade->epout_buf[1] + 1;
            tud_gh_arcade_set_side_cb(itf, side);
        }
        TU_ASSERT(usbd_edpt_xfer(rhport, p_gh_arcade->ep_out, p_gh_arcade->epout_buf,
                                 0x40));
    }
    return true;
}

#endif