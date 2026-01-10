
#include "tusb.h"
#include "common/tusb_types.h"
#include "device/usbd_pvt.h"
#include "hardware/structs/usb.h"
#include "usb/device/gh_arcade_device.h"
#include "usb/device/xinput_device.h"
#include "usb/device/xone_device.h"
#include "usb/device/ogxbox_device.h"
#include "usb/device/hid_device.h"
#include "usb/device/ps3_device.h"
#include "usb/device/ps4_device.h"
usbd_class_driver_t drivers[] = {
    {
#if CFG_TUSB_DEBUG >= 2
        .name = "Hid_Device",
#endif
        .init = hidd_init,
        .reset = hidd_reset,
        .open = hidd_open,
        .control_xfer_cb = hidd_control_xfer_cb,
        .xfer_cb = hidd_xfer_cb,
        .sof = NULL},
    {
#if CFG_TUSB_DEBUG >= 2
        .name = "XInput_Device",
#endif
        .init = xinputd_init,
        .reset = xinputd_reset,
        .open = xinputd_open,
        .control_xfer_cb = xinputd_control_xfer_cb,
        .xfer_cb = xinputd_xfer_cb,
        .sof = NULL},
    {
#if CFG_TUSB_DEBUG >= 2
        .name = "XOne_Device",
#endif
        .init = xoned_init,
        .reset = xoned_reset,
        .open = xoned_open,
        .control_xfer_cb = xoned_control_xfer_cb,
        .xfer_cb = xoned_xfer_cb,
        .sof = NULL},
    {
#if CFG_TUSB_DEBUG >= 2
        .name = "OgXbox_Device",
#endif
        .init = ogxboxd_init,
        .reset = ogxboxd_reset,
        .open = ogxboxd_open,
        .control_xfer_cb = ogxboxd_control_xfer_cb,
        .xfer_cb = ogxboxd_xfer_cb,
        .sof = NULL},
    {
#if CFG_TUSB_DEBUG >= 2
        .name = "GhArcade_Device",
#endif
        .init = gh_arcaded_init,
        .reset = gh_arcaded_reset,
        .open = gh_arcaded_open,
        .control_xfer_cb = gh_arcaded_control_xfer_cb,
        .xfer_cb = gh_arcaded_xfer_cb,
        .sof = NULL}};

usbd_class_driver_t const *usbd_app_driver_get_cb(uint8_t *driver_count)
{
    *driver_count = TU_ARRAY_SIZE(drivers);
    return drivers;
}

const OS_EXTENDED_COMPATIBLE_ID_DESCRIPTOR ExtendedIDs = {
    TotalLength : sizeof(OS_EXTENDED_COMPATIBLE_ID_DESCRIPTOR),
    Version : 0x0100,
    Index : DESC_EXTENDED_PROPERTIES_DESCRIPTOR,
    TotalSections : 1,
    SectionSize : 132,
    ExtendedID : {
        PropertyDataType : 1,
        PropertyNameLength : 40,
        PropertyName : {'D', 'e', 'v', 'i', 'c', 'e', 'I', 'n', 't', 'e',
                        'r', 'f', 'a', 'c', 'e', 'G', 'U', 'I', 'D', '\0'},
        PropertyDataLength : 78,
        PropertyData :
            {'{', 'D', 'F', '5', '9', '0', '3', '7', 'D', '-', '7', 'C', '9',
             '2', '-', '4', '1', '5', '5', '-', 'A', 'C', '1', '2', '-', '7',
             'D', '7', '0', '0', 'A', '3', '1', '3', 'D', '7', '9', '}', '\0'}
    }
};

bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request)
{
    // printf("control req %02x %02x %02x %02x %04x %04x\r\n", request->bmRequestType_bit.direction,request->bmRequestType_bit.type,request->bmRequestType_bit.recipient, request->bRequest, request->wIndex, request->wValue);
    if (request->bmRequestType_bit.direction == TUSB_DIR_IN)
    {
        if (stage == CONTROL_STAGE_SETUP)
        {
            if (request->bmRequestType_bit.type == TUSB_REQ_TYPE_VENDOR)
            {
                if (request->bmRequestType_bit.recipient == TUSB_REQ_RCPT_INTERFACE)
                {
                    if (request->bRequest == REQ_GET_OS_FEATURE_DESCRIPTOR && request->wIndex == DESC_EXTENDED_PROPERTIES_DESCRIPTOR)
                    {
                        tud_control_xfer(rhport, request, (void *)&ExtendedIDs, ExtendedIDs.TotalLength);
                        return true;
                    }
                }
            }
        }
    }
    for (auto &d : drivers)
    {
        if (d.control_xfer_cb(rhport, stage, request))
        {
            return true;
        }
    }
    return false;
}

uint8_t UsbDevice::m_last_epin = 0x81;
uint8_t UsbDevice::m_last_epout = 0x01;