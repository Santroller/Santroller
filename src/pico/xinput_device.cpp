#include "tusb_option.h"
// Combine together the implementation of xinput and hid as they are baiscally the same
#if (TUSB_OPT_DEVICE_ENABLED && CFG_TUD_XINPUT)

//--------------------------------------------------------------------+
// INCLUDE
//--------------------------------------------------------------------+
#include "class/hid/hid.h"
#include "common/tusb_common.h"
#include "config.h"
#include "descriptors.h"
#include "device/usbd_pvt.h"
#include "hid.h"
#include "xinput_device.h"

//--------------------------------------------------------------------+
// MACRO CONSTANT TYPEDEF
//--------------------------------------------------------------------+
typedef struct {
    uint8_t itf_num;
    uint8_t ep_in;
    uint8_t ep_out;         // optional Out endpoint
    uint8_t boot_protocol;  // Boot mouse or keyboard
    bool boot_mode;         // default = false (Report)

    CFG_TUSB_MEM_ALIGN uint8_t epin_buf[CFG_TUD_XINPUT_TX_BUFSIZE];
    CFG_TUSB_MEM_ALIGN uint8_t epout_buf[CFG_TUD_XINPUT_RX_BUFSIZE];
} xinputd_interface_t;

CFG_TUSB_MEM_SECTION static xinputd_interface_t _xinputd_itf[CFG_TUD_XINPUT];
static volatile bool sending = false;
/*------------- Helpers -------------*/
static inline uint8_t get_index_by_itfnum(uint8_t itf_num) {
    for (uint8_t i = 0; i < CFG_TUD_XINPUT; i++) {
        if (itf_num == _xinputd_itf[i].itf_num) return i;
    }

    return 0xFF;
}

//--------------------------------------------------------------------+
// APPLICATION API
//--------------------------------------------------------------------+
bool tud_xinput_n_ready(uint8_t itf) {
    uint8_t const ep_in = _xinputd_itf[itf].ep_in;
    return tud_ready() && (ep_in != 0) && !usbd_edpt_busy(TUD_OPT_RHPORT, ep_in);
}

bool tud_ready_for_packet(void) {
    return !sending;
}

bool tud_xusb_n_report(uint8_t itf, void const *report, uint8_t len) {
    uint8_t const rhport = 0;
    xinputd_interface_t *p_xinput = &_xinputd_itf[itf];

    // claim endpoint
    TU_VERIFY(usbd_edpt_claim(rhport, p_xinput->ep_in));

    memcpy(p_xinput->epin_buf, report, len);
    sending = true;
    return usbd_edpt_xfer(TUD_OPT_RHPORT, p_xinput->ep_in, p_xinput->epin_buf, len);
}

bool tud_xinput_n_report(uint8_t itf, uint8_t report_id, void const *report,
                         uint8_t len) {
    uint8_t const rhport = 0;
    xinputd_interface_t *p_xinput = &_xinputd_itf[itf];

    // claim endpoint
    TU_VERIFY(usbd_edpt_claim(rhport, p_xinput->ep_in));

    // prepare data
    if (report_id) {
        len = tu_min8(len, CFG_TUD_XINPUT_TX_BUFSIZE - 1);

        p_xinput->epin_buf[0] = report_id;
        memcpy(p_xinput->epin_buf + 1, report, len);
        len++;
    } else {
        // If report id = 0, skip ID field
        len = tu_min8(len, CFG_TUD_XINPUT_TX_BUFSIZE);
        memcpy(p_xinput->epin_buf, report, len);
    }
    sending = true;
    return usbd_edpt_xfer(TUD_OPT_RHPORT, p_xinput->ep_in, p_xinput->epin_buf,
                          len);
}

bool tud_xinput_n_boot_mode(uint8_t itf) { return _xinputd_itf[itf].boot_mode; }

//--------------------------------------------------------------------+
// USBD-CLASS API
//--------------------------------------------------------------------+
void xinputd_init(void) {
    xinputd_reset(TUD_OPT_RHPORT);
}

void xinputd_reset(uint8_t rhport) {
    (void)rhport;
    tu_memclr(_xinputd_itf, sizeof(_xinputd_itf));
    sending = false;
}

uint16_t xinputd_open(uint8_t rhport, tusb_desc_interface_t const *itf_desc,
                      uint16_t max_len) {
    uint16_t drv_len;
    if (TUSB_CLASS_VENDOR_SPECIFIC == itf_desc->bInterfaceClass) {
        TU_VERIFY(TUSB_CLASS_VENDOR_SPECIFIC == itf_desc->bInterfaceClass, 0);
        drv_len = sizeof(tusb_desc_interface_t) +
                  (itf_desc->bNumEndpoints * sizeof(tusb_desc_endpoint_t));

        TU_VERIFY(max_len >= drv_len, 0);

        // Find available interface
        xinputd_interface_t *p_xinput = NULL;
        for (uint8_t i = 0; i < CFG_TUD_XINPUT; i++) {
            if (_xinputd_itf[i].ep_in == 0 && _xinputd_itf[i].ep_out == 0) {
                p_xinput = &_xinputd_itf[i];
                break;
            }
        }
        TU_VERIFY(p_xinput, 0);
        uint8_t const *p_desc = (uint8_t const *)itf_desc;
        if (itf_desc->bInterfaceSubClass == 0x5D &&
            (itf_desc->bInterfaceProtocol == 0x01 ||
             itf_desc->bInterfaceProtocol == 0x03 ||
             itf_desc->bInterfaceProtocol == 0x02)) {
            // Xinput reserved endpoint
            //-------------- Xinput Descriptor --------------//
            p_desc = tu_desc_next(p_desc);
            XBOX_ID_DESCRIPTOR *x_desc =
                (XBOX_ID_DESCRIPTOR *)p_desc;
            TU_ASSERT(XINPUT_DESC_TYPE_RESERVED == x_desc->bDescriptorType, 0);
            drv_len += x_desc->bLength;
            p_desc = tu_desc_next(p_desc);
            TU_ASSERT(usbd_open_edpt_pair(rhport, p_desc, itf_desc->bNumEndpoints,
                                          TUSB_XFER_INTERRUPT, &p_xinput->ep_out,
                                          &p_xinput->ep_in),
                      0);

            p_xinput->itf_num = itf_desc->bInterfaceNumber;
            if (p_xinput->ep_out) {
                if (!usbd_edpt_xfer(rhport, p_xinput->ep_out, p_xinput->epout_buf, sizeof(p_xinput->epout_buf))) {
                    TU_LOG_FAILED();
                    TU_BREAKPOINT();
                }
            }
        } else if (itf_desc->bInterfaceSubClass == 0xfD &&
                   itf_desc->bInterfaceProtocol == 0x13) {
            // Xinput reserved endpoint
            //-------------- Xinput Descriptor --------------//
            p_desc = tu_desc_next(p_desc);
            XBOX_ID_DESCRIPTOR *x_desc =
                (XBOX_ID_DESCRIPTOR *)p_desc;
            TU_ASSERT(XINPUT_SECURITY_DESC_TYPE_RESERVED == x_desc->bDescriptorType, 0);
            drv_len += x_desc->bLength;
            p_desc = tu_desc_next(p_desc);
        } else if (itf_desc->bInterfaceSubClass == 0x47 &&
                   itf_desc->bInterfaceProtocol == 0xD0) {
            p_desc = tu_desc_next(p_desc);
            TU_ASSERT(usbd_open_edpt_pair(rhport, p_desc, itf_desc->bNumEndpoints, TUSB_XFER_INTERRUPT, &p_xinput->ep_out, &p_xinput->ep_in), 0);

            p_xinput->itf_num = itf_desc->bInterfaceNumber;

            // Prepare for output endpoint
            if (p_xinput->ep_out) {
                if (!usbd_edpt_xfer(rhport, p_xinput->ep_out, p_xinput->epout_buf, sizeof(p_xinput->epout_buf))) {
                    TU_LOG_FAILED();
                    TU_BREAKPOINT();
                }
            }
        }
    } else {
        TU_VERIFY(TUSB_CLASS_HID == itf_desc->bInterfaceClass, 0);

        // len = interface + hid + n*endpoints
        drv_len = sizeof(tusb_desc_interface_t) + sizeof(tusb_hid_descriptor_hid_t) + itf_desc->bNumEndpoints * sizeof(tusb_desc_endpoint_t);
        TU_ASSERT(max_len >= drv_len, 0);

        // Find available interface
        xinputd_interface_t *p_hid = NULL;
        uint8_t hid_id;
        for (hid_id = 0; hid_id < CFG_TUD_XINPUT; hid_id++) {
            if (_xinputd_itf[hid_id].ep_in == 0) {
                p_hid = &_xinputd_itf[hid_id];
                break;
            }
        }
        TU_ASSERT(p_hid, 0);

        uint8_t const *p_desc = (uint8_t const *)itf_desc;

        //------------- HID descriptor -------------//
        p_desc = tu_desc_next(p_desc);
        TU_ASSERT(HID_DESC_TYPE_HID == ((tusb_hid_descriptor_hid_t const *)p_desc)->bDescriptorType, 0);

        //------------- Endpoint Descriptor -------------//
        p_desc = tu_desc_next(p_desc);
        TU_ASSERT(usbd_open_edpt_pair(rhport, p_desc, itf_desc->bNumEndpoints, TUSB_XFER_INTERRUPT, &p_hid->ep_out, &p_hid->ep_in), 0);

        p_hid->itf_num = itf_desc->bInterfaceNumber;

        // Prepare for output endpoint
        if (p_hid->ep_out) {
            if (!usbd_edpt_xfer(rhport, p_hid->ep_out, p_hid->epout_buf, sizeof(p_hid->epout_buf))) {
                TU_LOG_FAILED();
                TU_BREAKPOINT();
            }
        }
    }
    //------------- Endpoint Descriptor -------------//

    // Config endpoint

    return drv_len;
}

bool xinputd_xfer_cb(uint8_t rhport, uint8_t ep_addr, xfer_result_t result,
                     uint32_t xferred_bytes) {
    (void)result;

    uint8_t itf = 0;
    xinputd_interface_t *p_xinput = _xinputd_itf;

    for (;; itf++, p_xinput++) {
        if (itf >= TU_ARRAY_SIZE(_xinputd_itf)) return false;

        if (ep_addr == p_xinput->ep_out || ep_addr == p_xinput->ep_in) break;
    }

    if (ep_addr == p_xinput->ep_out) {
        hid_set_report(p_xinput->epout_buf, xferred_bytes, 0x00, INTERRUPT_ID);
        if (consoleType == XBOX360 || consoleType == WINDOWS) {
            TU_ASSERT(usbd_edpt_xfer(rhport, p_xinput->ep_out, p_xinput->epout_buf,
                                     0x20));

#if USB_HOST_STACK
        } else if (consoleType == XBOXONE) {
            TU_ASSERT(usbd_edpt_xfer(rhport, p_xinput->ep_out, p_xinput->epout_buf,
                                     0x40));
#endif
        } else {
            TU_ASSERT(usbd_edpt_xfer(rhport, p_xinput->ep_out, p_xinput->epout_buf,
                                     0x08));
        }

    } else if (ep_addr == p_xinput->ep_in) {
        sending = false;
    }
    return true;
}

#endif