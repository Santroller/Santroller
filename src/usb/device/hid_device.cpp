#include "usb/device/hid_device.h"
#include "usb/device/gamepad_device.h"
#include "enums.pb.h"

const OS_COMPATIBLE_ID_DESCRIPTOR DevCompatIDsUniversal = {
    TotalLength : sizeof(OS_COMPATIBLE_ID_DESCRIPTOR),
    Version : 0x0100,
    Index : DESC_EXTENDED_COMPATIBLE_ID_DESCRIPTOR,
    TotalSections : 2,
    Reserved : {0},
    CompatID : {
        {
            FirstInterfaceNumber : ITF_NUM_XONE,
            Reserved : 0x01,
            CompatibleID : "XGIP10",
            SubCompatibleID : {0},
            Reserved2 : {0}
        },
        {
            FirstInterfaceNumber : ITF_NUM_XINPUT_SECURITY,
            Reserved : 0x01,
            CompatibleID : "WINUSB",
            SubCompatibleID : {0},
            Reserved2 : {0}
        },}
};

uint16_t tud_hid_generic_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
{
    (void)instance;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)reqlen;
    if (report_type != HID_REPORT_TYPE_FEATURE)
    {
        return 0;
    }

    return 0;
}

void tud_hid_generic_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize)
{
    (void)instance;
}

bool tud_hid_generic_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request)
{
    if (request->bmRequestType_bit.direction == TUSB_DIR_IN)
    {
        if (request->bmRequestType_bit.type == TUSB_REQ_TYPE_VENDOR && request->bRequest == REQ_GET_OS_FEATURE_DESCRIPTOR)
        {
            if (request->bmRequestType_bit.recipient == TUSB_REQ_RCPT_DEVICE)
            {
                if (request->bRequest == REQ_GET_OS_FEATURE_DESCRIPTOR && request->wIndex == DESC_EXTENDED_COMPATIBLE_ID_DESCRIPTOR)
                {
                    if (stage == CONTROL_STAGE_SETUP)
                    {
                        tud_control_xfer(rhport, request, (void *)&DevCompatIDsUniversal, sizeof(OS_COMPATIBLE_ID_DESCRIPTOR));
                    }
                    return true;
                }
            }
        }
    }
    return false;
}