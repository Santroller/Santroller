/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * This file is part of the TinyUSB stack.
 */

#include "tusb_option.h"

#if (CFG_TUH_ENABLED && CFG_TUH_XINPUT)
#include "class/hid/hid.h"
#include "defines.h"
#include "descriptors.h"
#include "hid.h"
#include "hidparser.h"
#include "host/usbh.h"
#include "host/usbh_classdriver.h"
#include "xinput_host.h"

#define INVALID_REPORT_ID -1
//--------------------------------------------------------------------+
// MACRO CONSTANT TYPEDEF
//--------------------------------------------------------------------+

typedef struct
{
    uint8_t itf_num;
    uint8_t ep_in;
    uint8_t ep_out;
    uint8_t type;
    uint8_t subtype;

    uint16_t epin_size;
    uint16_t epout_size;

    uint8_t epin_buf[CFG_TUH_XINPUT_EPIN_BUFSIZE];
    uint8_t epout_buf[CFG_TUH_XINPUT_EPOUT_BUFSIZE];
    HID_ReportInfo_t *info;
} xinputh_interface_t;

typedef struct
{
    uint8_t inst_count;
    xinputh_interface_t instances[CFG_TUH_XINPUT];
} xinputh_device_t;
static xinputh_device_t _xinputh_dev[CFG_TUH_DEVICE_MAX];

//------------- Internal prototypes -------------//

// Get HID device & interface
TU_ATTR_ALWAYS_INLINE static inline xinputh_device_t *get_dev(uint8_t dev_addr);
TU_ATTR_ALWAYS_INLINE static inline xinputh_interface_t *get_instance(uint8_t dev_addr, uint8_t instance);
static uint8_t get_instance_id_by_itfnum(uint8_t dev_addr, uint8_t itf);
static uint8_t get_instance_id_by_epaddr(uint8_t dev_addr, uint8_t ep_addr);
static int16_t reportID;
static bool foundPS5 = false;
static bool foundPS4 = false;
static bool foundPS3 = false;
//--------------------------------------------------------------------+
// Interface API
//--------------------------------------------------------------------+

uint8_t tuh_xinput_instance_count(uint8_t dev_addr) {
    return get_dev(dev_addr)->inst_count;
}

bool tuh_xinput_mounted(uint8_t dev_addr, uint8_t instance) {
    if (get_dev(dev_addr)->inst_count < instance) return false;
    xinputh_interface_t *hid_itf = get_instance(dev_addr, instance);
    return (hid_itf->ep_in != 0) || (hid_itf->ep_out != 0);
}

//--------------------------------------------------------------------+
// Interrupt Endpoint API
//--------------------------------------------------------------------+

bool tuh_xinput_receive_report(uint8_t dev_addr, uint8_t instance) {
    xinputh_interface_t *hid_itf = get_instance(dev_addr, instance);

    // claim endpoint
    TU_VERIFY(usbh_edpt_claim(dev_addr, hid_itf->ep_in));

    if (!usbh_edpt_xfer(dev_addr, hid_itf->ep_in, hid_itf->epin_buf, hid_itf->epin_size)) {
        usbh_edpt_release(dev_addr, hid_itf->ep_in);
        return false;
    }

    return true;
}

bool tuh_xinput_ready(uint8_t dev_addr, uint8_t instance) {
    TU_VERIFY(tuh_xinput_mounted(dev_addr, instance));

    xinputh_interface_t *hid_itf = get_instance(dev_addr, instance);
    return !usbh_edpt_busy(dev_addr, hid_itf->ep_in);
}

bool tuh_xinput_send_report(uint8_t dev_addr, uint8_t instance, uint8_t const *report, uint16_t len) {
    xinputh_interface_t *hid_itf = get_instance(dev_addr, instance);
    // claim endpoint
    TU_VERIFY(usbh_edpt_claim(dev_addr, hid_itf->ep_out));
    memcpy(hid_itf->epout_buf, report, len);
    if (!usbh_edpt_xfer(dev_addr, hid_itf->ep_out, hid_itf->epout_buf, len)) {
        usbh_edpt_release(dev_addr, hid_itf->ep_out);
        return false;
    }
    return true;
}

//--------------------------------------------------------------------+
// USBH API
//--------------------------------------------------------------------+
void xinputh_init(void) {
    tu_memclr(_xinputh_dev, sizeof(_xinputh_dev));
}

static inline bool USB_GetHIDReportItemInfoWithReportId(const uint8_t *ReportData, HID_ReportItem_t *const ReportItem) {
    if (ReportItem->ReportID) {
        if (ReportItem->ReportID != ReportData[0])
            return false;

        ReportData++;
    }
    return USB_GetHIDReportItemInfo(ReportItem->ReportID, ReportData, ReportItem);
}

uint16_t GetAxis(HID_ReportItem_t *item) {
    uint8_t size = item->Attributes.BitSize;
    uint32_t val = item->Value;
    if (size > 16) {
        val >>= size - 16;
    } else if (size < 16) {
        val <<= 16 - size;
    }
    return val;
}

void fill_generic_report(uint8_t dev_addr, uint8_t instance, const uint8_t *report, USB_Host_Data_t *out) {
    memset(out, 0, sizeof(USB_Host_Data_t));
    xinputh_interface_t *hid_itf = get_instance(dev_addr, instance);
    if (hid_itf->info != NULL) {
        HID_ReportItem_t *item = hid_itf->info->FirstReportItem;
        while (item) {
            if (USB_GetHIDReportItemInfoWithReportId(report, item)) {
                switch (item->Attributes.Usage.Page) {
                    case HID_USAGE_PAGE_DESKTOP:
                        switch (item->Attributes.Usage.Usage) {
                            case HID_USAGE_DESKTOP_X:
                                out->genericX = GetAxis(item);
                                break;
                            case HID_USAGE_DESKTOP_Y:
                                out->genericY = GetAxis(item);
                                break;
                            case HID_USAGE_DESKTOP_Z:
                                out->genericZ = GetAxis(item);
                                break;
                            case HID_USAGE_DESKTOP_RX:
                                out->genericRX = GetAxis(item);
                                break;
                            case HID_USAGE_DESKTOP_RY:
                                out->genericRY = GetAxis(item);
                                break;
                            case HID_USAGE_DESKTOP_RZ:
                                out->genericRZ = GetAxis(item);
                                break;
                            case HID_USAGE_DESKTOP_SLIDER:
                                out->genericSlider = GetAxis(item);
                                break;
                            case HID_USAGE_DESKTOP_HAT_SWITCH:
                                out->dpadLeft = item->Value == 6 || item->Value == 5 || item->Value == 7;
                                out->dpadRight = item->Value == 3 || item->Value == 2 || item->Value == 1;
                                out->dpadUp = item->Value == 0 || item->Value == 1 || item->Value == 7;
                                out->dpadDown = item->Value == 5 || item->Value == 4 || item->Value == 3;
                                break;
                            case HID_USAGE_DESKTOP_DPAD_UP:
                                out->dpadUp = 1;
                                break;
                            case HID_USAGE_DESKTOP_DPAD_RIGHT:
                                out->dpadRight = 1;
                                break;
                            case HID_USAGE_DESKTOP_DPAD_DOWN:
                                out->dpadDown = 1;
                                break;
                            case HID_USAGE_DESKTOP_DPAD_LEFT:
                                out->dpadLeft = 1;
                                break;
                        }
                        break;
                    case HID_USAGE_PAGE_BUTTON: {
                        uint8_t usage = item->Attributes.Usage.Usage;
                        if (usage <= 16 && item->Value) {
                            out->genericButtons |= 1 << usage - 1;
                        }
                        break;
                    }
                }
            }
            item = item->Next;
        }
    }
}
bool xinputh_xfer_cb(uint8_t dev_addr, uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes) {
    (void)result;

    uint8_t const dir = tu_edpt_dir(ep_addr);
    uint8_t const instance = get_instance_id_by_epaddr(dev_addr, ep_addr);
    xinputh_interface_t *hid_itf = get_instance(dev_addr, instance);
    if (dir == TUSB_DIR_IN) {
        usbh_edpt_xfer(dev_addr, hid_itf->ep_in, hid_itf->epin_buf, hid_itf->epin_size);
        tuh_xinput_report_received_cb(dev_addr, instance, hid_itf->epin_buf, (uint16_t)xferred_bytes);
    } else {
        tuh_xinput_report_sent_cb(dev_addr, instance, hid_itf->epin_buf, (uint16_t)xferred_bytes);
    }

    return true;
}

void xinputh_close(uint8_t dev_addr) {
    TU_VERIFY(dev_addr <= CFG_TUH_DEVICE_MAX, );

    xinputh_device_t *hid_dev = get_dev(dev_addr);

    for (uint8_t inst = 0; inst < hid_dev->inst_count; inst++) {
        if (tuh_xinput_umount_cb) {
            tuh_xinput_umount_cb(dev_addr, inst);
        }
        if (hid_dev->instances[inst].info != NULL) {
            USB_FreeReportInfo(hid_dev->instances[inst].info);
        }
    }

    tu_memclr(hid_dev, sizeof(xinputh_device_t));
}

bool CALLBACK_HIDParser_FilterHIDReportItem(HID_ReportItem_t *const CurrentItem) {
    if (CurrentItem->ItemType == HID_REPORT_ITEM_Feature && CurrentItem->Attributes.Usage.Page == HID_USAGE_PAGE_VENDOR) {
        if (CurrentItem->Attributes.Usage.Usage == 0x2821) {
            foundPS5 = true;
        }
        if (CurrentItem->Attributes.Usage.Usage == 0x2721) {
            foundPS4 = true;
        }
        if (CurrentItem->Attributes.Usage.Usage == 0x2621) {
            foundPS3 = true;
        }
    }

    if (CurrentItem->ItemType != HID_REPORT_ITEM_In)
        return false;

    if (reportID == INVALID_REPORT_ID) {
        reportID = CurrentItem->ReportID;
    }
    switch (CurrentItem->Attributes.Usage.Page) {
        case HID_USAGE_PAGE_DESKTOP:
            switch (CurrentItem->Attributes.Usage.Usage) {
                case HID_USAGE_DESKTOP_X:
                case HID_USAGE_DESKTOP_Y:
                case HID_USAGE_DESKTOP_Z:
                case HID_USAGE_DESKTOP_RZ:
                case HID_USAGE_DESKTOP_HAT_SWITCH:
                case HID_USAGE_DESKTOP_DPAD_UP:
                case HID_USAGE_DESKTOP_DPAD_DOWN:
                case HID_USAGE_DESKTOP_DPAD_LEFT:
                case HID_USAGE_DESKTOP_DPAD_RIGHT:
                    return true;
            }
            return false;
        case HID_USAGE_PAGE_BUTTON:
            return true;
    }
    return false;
}

//--------------------------------------------------------------------+
// Enumeration
//--------------------------------------------------------------------+
extern uint8_t transfer_with_usb_controller(const uint8_t dev_addr, const uint8_t requestType, const uint8_t request, const uint16_t wValue, const uint16_t wIndex, const uint16_t wLength, uint8_t *buffer);
bool xinputh_open(uint8_t rhport, uint8_t dev_addr, tusb_desc_interface_t const *desc_itf, uint16_t max_len) {
    (void)rhport;
    (void)max_len;
    if (dev_addr == 9) {
        return false;
    }
    // Some controllers need a delay here otherwise they don't initialise properly
    sleep_ms(1);
    TU_VERIFY(TUSB_CLASS_VENDOR_SPECIFIC == desc_itf->bInterfaceClass || TUSB_CLASS_HID == desc_itf->bInterfaceClass, 0);
    xinputh_interface_t *p_xinput = NULL;
    uint8_t i = 0;
    for (; i < CFG_TUH_XINPUT; i++) {
        xinputh_interface_t *hid_itf = get_instance(dev_addr, i);
        if (hid_itf->ep_in == 0 && hid_itf->ep_out == 0) {
            p_xinput = hid_itf;
            break;
        }
    }
    TU_VERIFY(p_xinput, 0);
    uint16_t drv_len = sizeof(tusb_desc_interface_t) +
                       (desc_itf->bNumEndpoints * sizeof(tusb_desc_endpoint_t));
    // Support standard HID devices
    if (TUSB_CLASS_HID == desc_itf->bInterfaceClass) {
        TU_VERIFY(TUSB_CLASS_HID == desc_itf->bInterfaceClass, 0);
        uint8_t const *p_desc = (uint8_t const *)desc_itf;
        // Xinput reserved endpoint
        //-------------- Xinput Descriptor --------------//
        p_desc = tu_desc_next(p_desc);
        USB_HID_DESCRIPTOR *x_desc =
            (USB_HID_DESCRIPTOR *)p_desc;
        TU_ASSERT(HID_DESCRIPTOR_HID == x_desc->bDescriptorType, 0);
        drv_len += x_desc->bLength;
        uint8_t endpoints = desc_itf->bNumEndpoints;
        while (endpoints--) {
            p_desc = tu_desc_next(p_desc);
            tusb_desc_endpoint_t const *desc_ep =
                (tusb_desc_endpoint_t const *)p_desc;
            TU_ASSERT(TUSB_DESC_ENDPOINT == desc_ep->bDescriptorType);
            if (desc_ep->bEndpointAddress & 0x80) {
                p_xinput->ep_in = desc_ep->bEndpointAddress;
                p_xinput->epin_size = desc_ep->wMaxPacketSize;
                TU_ASSERT(tuh_edpt_open(dev_addr, desc_ep));
                usbh_edpt_xfer(dev_addr, p_xinput->ep_in, p_xinput->epin_buf, p_xinput->epin_size);
            } else {
                p_xinput->ep_out = desc_ep->bEndpointAddress;
                p_xinput->epout_size = desc_ep->wMaxPacketSize;
                TU_ASSERT(tuh_edpt_open(dev_addr, desc_ep));
            }
        }
        p_xinput->itf_num = desc_itf->bInterfaceNumber;

        p_xinput->type = UNKNOWN;
        if (desc_itf->bInterfaceProtocol == HID_ITF_PROTOCOL_KEYBOARD) {
            p_xinput->type = KEYBOARD;
        }
        if (desc_itf->bInterfaceProtocol == HID_ITF_PROTOCOL_MOUSE) {
            p_xinput->type = MOUSE;
        }
        // TODO: do we support nkro
        if (p_xinput->type == UNKNOWN) {
            uint8_t temp_buf[512];
            uint8_t *current = temp_buf;
            uint8_t len = x_desc->wDescriptorLength;
            uint8_t last_id = 0;
            tuh_descriptor_get_hid_report(dev_addr, p_xinput->itf_num, x_desc->bDescrType, 0, temp_buf, x_desc->wDescriptorLength, NULL, (uintptr_t)temp_buf);
            // Seems that sometimes we miss the first byte?
            if (!temp_buf[0]) {
                temp_buf[0] = 0x05;
            }
            foundPS3 = false;
            foundPS4 = false;
            foundPS5 = false;
            reportID = INVALID_REPORT_ID;
            USB_ProcessHIDReport(temp_buf, x_desc->wDescriptorLength, &p_xinput->info);
            if (foundPS5) {
                p_xinput->type = PS5;
            }
            if (foundPS4) {
                p_xinput->type = PS4;
            }
            if (foundPS3) {
                p_xinput->type = PS3;
            }
        }
        _xinputh_dev->inst_count++;
        tuh_xinput_receive_report(dev_addr, i);
        p_xinput->subtype = GAMEPAD;
        return true;
    }

    uint8_t const *p_desc = (uint8_t const *)desc_itf;
    if (desc_itf->bInterfaceSubClass == 0x5D &&
        desc_itf->bInterfaceProtocol == 0x04) {
        // Big button
        uint8_t endpoints = desc_itf->bNumEndpoints;
        while (endpoints--) {
            p_desc = tu_desc_next(p_desc);
            tusb_desc_endpoint_t const *desc_ep =
                (tusb_desc_endpoint_t const *)p_desc;
            TU_ASSERT(TUSB_DESC_ENDPOINT == desc_ep->bDescriptorType);
            if (desc_ep->bEndpointAddress & 0x80) {
                p_xinput->ep_in = desc_ep->bEndpointAddress;
                p_xinput->epin_size = desc_ep->wMaxPacketSize;
                TU_ASSERT(tuh_edpt_open(dev_addr, desc_ep));
                usbh_edpt_xfer(dev_addr, p_xinput->ep_in, p_xinput->epin_buf, p_xinput->epin_size);
            } else {
                p_xinput->ep_out = desc_ep->bEndpointAddress;
                p_xinput->epout_size = desc_ep->wMaxPacketSize;
                TU_ASSERT(tuh_edpt_open(dev_addr, desc_ep));
            }
        }
        p_xinput->itf_num = desc_itf->bInterfaceNumber;

        p_xinput->type = XBOX360_BB;
        p_xinput->subtype = GAMEPAD;
    } else if (desc_itf->bInterfaceSubClass == 0x5D &&
               (desc_itf->bInterfaceProtocol == 0x01 ||
                desc_itf->bInterfaceProtocol == 0x03 ||
                desc_itf->bInterfaceProtocol == 0x02)) {
        // Xinput reserved endpoint
        //-------------- Xinput Descriptor --------------//
        p_desc = tu_desc_next(p_desc);
        XBOX_ID_DESCRIPTOR *x_desc =
            (XBOX_ID_DESCRIPTOR *)p_desc;
        TU_ASSERT(XINPUT_DESC_TYPE_RESERVED == x_desc->bDescriptorType, 0);
        drv_len += x_desc->bLength;
        uint8_t endpoints = desc_itf->bNumEndpoints;
        while (endpoints--) {
            p_desc = tu_desc_next(p_desc);
            tusb_desc_endpoint_t const *desc_ep =
                (tusb_desc_endpoint_t const *)p_desc;
            TU_ASSERT(TUSB_DESC_ENDPOINT == desc_ep->bDescriptorType);
            if (desc_ep->bEndpointAddress & 0x80) {
                p_xinput->ep_in = desc_ep->bEndpointAddress;
                TU_ASSERT(tuh_edpt_open(dev_addr, desc_ep));
            } else {
                p_xinput->ep_out = desc_ep->bEndpointAddress;
                TU_ASSERT(tuh_edpt_open(dev_addr, desc_ep));
            }
        }
        p_xinput->itf_num = desc_itf->bInterfaceNumber;
        p_xinput->type = XBOX360;
        p_xinput->subtype = GAMEPAD;
        if (desc_itf->bInterfaceProtocol == 0x01) {
            p_xinput->subtype = x_desc->subtype;
            usbh_edpt_xfer(dev_addr, p_xinput->ep_in, p_xinput->epin_buf, p_xinput->epin_size);
        } else {
            p_xinput->type = NON_CONTROLLER;
        }
        _xinputh_dev->inst_count++;
        return true;

    } else if (desc_itf->bInterfaceSubClass == 0xfD &&
               desc_itf->bInterfaceProtocol == 0x13) {
        // Xinput reserved endpoint
        //-------------- Xinput Descriptor --------------//
        p_desc = tu_desc_next(p_desc);
        XBOX_ID_DESCRIPTOR *x_desc =
            (XBOX_ID_DESCRIPTOR *)p_desc;
        TU_ASSERT(XINPUT_SECURITY_DESC_TYPE_RESERVED == x_desc->bDescriptorType, 0);
        drv_len += x_desc->bLength;
        p_desc = tu_desc_next(p_desc);
        p_xinput->type = NON_CONTROLLER;
        p_xinput->subtype = GAMEPAD;
        p_xinput->itf_num = desc_itf->bInterfaceNumber;
        _xinputh_dev->inst_count++;
        return true;
    } else if (desc_itf->bInterfaceSubClass == 0x47 &&
               desc_itf->bInterfaceProtocol == 0xD0 && desc_itf->bNumEndpoints) {
        uint8_t endpoints = desc_itf->bNumEndpoints;
        while (endpoints--) {
            p_desc = tu_desc_next(p_desc);
            tusb_desc_endpoint_t const *desc_ep =
                (tusb_desc_endpoint_t const *)p_desc;
            TU_ASSERT(TUSB_DESC_ENDPOINT == desc_ep->bDescriptorType);
            if (desc_ep->bEndpointAddress & 0x80) {
                p_xinput->ep_in = desc_ep->bEndpointAddress;
                TU_ASSERT(tuh_edpt_open(dev_addr, desc_ep));
            } else {
                p_xinput->ep_out = desc_ep->bEndpointAddress;
                TU_ASSERT(tuh_edpt_open(dev_addr, desc_ep));
            }
        }
        p_xinput->itf_num = desc_itf->bInterfaceNumber;
        p_xinput->type = XBOXONE;
        p_xinput->subtype = GAMEPAD;
        _xinputh_dev->inst_count++;
        usbh_edpt_xfer(dev_addr, p_xinput->ep_in, p_xinput->epin_buf, p_xinput->epin_size);
        return true;
    } else if (desc_itf->bInterfaceSubClass == 0x5D &&
               desc_itf->bInterfaceProtocol == 0x81) {
        // Skip over 0x22 descriptor
        p_desc = tu_desc_next(p_desc);
        uint8_t endpoints = desc_itf->bNumEndpoints;
        while (endpoints--) {
            p_desc = tu_desc_next(p_desc);
            tusb_desc_endpoint_t const *desc_ep =
                (tusb_desc_endpoint_t const *)p_desc;
            TU_ASSERT(TUSB_DESC_ENDPOINT == desc_ep->bDescriptorType);
            if (desc_ep->bEndpointAddress & 0x80) {
                p_xinput->ep_in = desc_ep->bEndpointAddress;
                TU_ASSERT(tuh_edpt_open(dev_addr, desc_ep));
            } else {
                p_xinput->ep_out = desc_ep->bEndpointAddress;
                TU_ASSERT(tuh_edpt_open(dev_addr, desc_ep));
            }
        }
        p_xinput->itf_num = desc_itf->bInterfaceNumber;
        p_xinput->type = XBOX360_W;
        p_xinput->subtype = UNKNOWN;
        _xinputh_dev->inst_count++;
        usbh_edpt_xfer(dev_addr, p_xinput->ep_in, p_xinput->epin_buf, p_xinput->epin_size);
        return true;
    }
    return false;
}

//--------------------------------------------------------------------+
// Set Configure
//--------------------------------------------------------------------+

enum {
    CONFG_SET_IDLE,
    CONFIG_SET_PROTOCOL,
    CONFIG_GET_REPORT_DESC,
    CONFIG_COMPLETE
};

static void config_driver_mount_complete(uint8_t dev_addr, uint8_t instance);
static void process_set_config(tuh_xfer_t *xfer);

bool xinputh_set_config(uint8_t dev_addr, uint8_t itf_num) {
    uint8_t instance = get_instance_id_by_itfnum(dev_addr, itf_num);
    config_driver_mount_complete(dev_addr, instance);
    return true;
}

static void config_driver_mount_complete(uint8_t dev_addr, uint8_t instance) {
    xinputh_interface_t *hid_itf = get_instance(dev_addr, instance);
    // enumeration is complete
    tuh_xinput_mount_cb(dev_addr, instance, hid_itf->type, hid_itf->subtype);

    // notify usbh that driver enumeration is complete
    usbh_driver_set_config_complete(dev_addr, hid_itf->itf_num);
}

//--------------------------------------------------------------------+
// Helper
//--------------------------------------------------------------------+

// Get Device by address
TU_ATTR_ALWAYS_INLINE static inline xinputh_device_t *get_dev(uint8_t dev_addr) {
    return &_xinputh_dev[dev_addr - 1];
}

// Get Interface by instance number
TU_ATTR_ALWAYS_INLINE static inline xinputh_interface_t *get_instance(uint8_t dev_addr, uint8_t instance) {
    return &_xinputh_dev[dev_addr - 1].instances[instance];
}

// Get instance ID by interface number
static uint8_t get_instance_id_by_itfnum(uint8_t dev_addr, uint8_t itf) {
    for (uint8_t inst = 0; inst < CFG_TUH_XINPUT; inst++) {
        xinputh_interface_t *hid = get_instance(dev_addr, inst);

        if ((hid->itf_num == itf)) return inst;
    }

    return 0xff;
}

// Get instance ID by endpoint address
static uint8_t get_instance_id_by_epaddr(uint8_t dev_addr, uint8_t ep_addr) {
    for (uint8_t inst = 0; inst < CFG_TUH_XINPUT; inst++) {
        xinputh_interface_t *hid = get_instance(dev_addr, inst);

        if ((ep_addr == hid->ep_in) || (ep_addr == hid->ep_out)) return inst;
    }

    return 0xff;
}

#endif
