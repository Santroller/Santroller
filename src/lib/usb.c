#include "usb.h"

#include <stddef.h>

#include "defines.h"
#include "descriptors.h"
#include "lib_main.h"
#include "report_descriptors.h"
#include "usb/xinput_descriptors.h"
uint16_t controlRequest(const requestType_t requestType, const uint8_t request, const uint16_t wValue, const uint16_t wIndex, const uint16_t wLength, uint8_t **address) {
    if (requestType.bmRequestType_bit.direction == USB_DIR_DEVICE_TO_HOST) {
        if (requestType.bmRequestType_bit.type == USB_REQ_TYPE_VENDOR) {
            if (requestType.bmRequestType_bit.recipient == USB_REQ_RCPT_INTERFACE) {
                if (request == THID_REQ_GetReport && wIndex == INTERFACE_ID_Device && wValue == 0x0000) {
                    *address = capabilities1;
                    return sizeof(capabilities1);
                } else if (request == REQ_GET_OS_FEATURE_DESCRIPTOR && wIndex == DESC_EXTENDED_PROPERTIES_DESCRIPTOR && wValue == INTERFACE_ID_Config) {
                    *address = (uint8_t *)&ExtendedIDs;
                    return ExtendedIDs.TotalLength;
                } else if (request == THID_REQ_GetReport && wIndex == INTERFACE_ID_Device && wValue == 0x0100) {
                    *address = capabilities2;
                    return sizeof(capabilities2);
                }
            } else if (requestType.bmRequestType_bit.recipient == USB_REQ_RCPT_DEVICE) {
                if (request == REQ_GET_OS_FEATURE_DESCRIPTOR && wIndex == DESC_EXTENDED_COMPATIBLE_ID_DESCRIPTOR) {
                    *address = (uint8_t *)&DevCompatIDs;
                    return DevCompatIDs.TotalLength;
                } else if (request == THID_REQ_GetReport && wIndex == 0x00 && wValue == 0x0000) {
                    *address = ID;
                    return sizeof(ID);
                }
            }
        } else if (requestType.bmRequestType_bit.type == USB_REQ_TYPE_CLASS && requestType.bmRequestType_bit.recipient == USB_REQ_RCPT_INTERFACE) {
            // hid read feature report received. Do we actually want to use this for the config api, or do we want to do something custom since we have raw usb access?
            // 
        }
    } else {
        // uint8_t *data = *address;
        if (requestType.bmRequestType_bit.type == USB_REQ_TYPE_CLASS && requestType.bmRequestType_bit.recipient == USB_REQ_RCPT_INTERFACE) {
            // hid write feature report received. Do we actually want to use this for the config api, or do we want to do something custom since we have raw usb access?
            // 
        }
    }
    return 0;
}
// bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage,
//                                 tusb_control_request_t const *request) {
//   } else if (request->bRequest == HID_REQ_SetReport &&
//              request->bmRequestType ==
//                  (REQDIR_HOSTTODEVICE | REQTYPE_CLASS | REQREC_INTERFACE)) {
//     if (stage == CONTROL_STAGE_SETUP) {
//       tud_control_xfer(rhport, request, buf, request->wLength);
//     } else if (stage == CONTROL_STAGE_ACK) {
//       int cmd = request->wValue;
//       processHIDWriteFeatureReport(cmd, request->wLength, buf);
//       if (isRF) {
//         uint8_t buf2[32];
//         uint8_t buf3[32];
//         memcpy(buf3 + 2, buf, 30);
//         buf3[0] = cmd;
//         buf3[1] = false;
//         while (nrf24_txFifoFull()) {
//           rf_interrupt = true;
//           tickRFInput(buf2, 0);
//           nrf24_configRegister(STATUS, (1 << TX_DS) | (1 << MAX_RT));
//         }
//         nrf24_configRegister(STATUS, (1 << TX_DS) | (1 << MAX_RT));
//         nrf24_writeAckPayload(buf3, sizeof(buf3));
//         rf_interrupt = true;
//       }
//     }
//   } else if (request->bRequest == HID_REQ_GetReport &&
//              request->bmRequestType ==
//                  (REQDIR_DEVICETOHOST | REQTYPE_CLASS | REQREC_INTERFACE)) {
//     if (stage == CONTROL_STAGE_SETUP) {
//       processHIDReadFeatureReport(request->wValue, rhport, request);
//     }
//   } else if (request->bmRequestType_bit.type == TUSB_REQ_TYPE_VENDOR) {
//     if (request->bmRequestType_bit.direction == TUSB_DIR_OUT &&
//         stage == CONTROL_STAGE_SETUP) {
//       tud_control_xfer(rhport, request, buf, request->wLength);
//     } else if ((request->bmRequestType_bit.direction == TUSB_DIR_IN &&
//                 stage == CONTROL_STAGE_SETUP) ||
//                request->bmRequestType_bit.direction == TUSB_DIR_OUT &&
//                    stage == CONTROL_STAGE_ACK) {
//       uint16_t len = sizeof(tusb_control_request_t);
//       uint8_t *b = (uint8_t *)request;
//       printf("control out: %d ", len);
//       while (len--) {
//         printf("%x ", *b);
//         uart_putc_raw(UART_ID, *(b++));
//       }
//       printf("\n");
//       len = request->wLength;
//       // device to host
//       if (request->bmRequestType_bit.direction == TUSB_DIR_IN) {
//         printf("dir_in\n");
//         printf("data read: ");
//         while (buf[0] != 0x7f) { uart_read_blocking(UART_ID, buf, 1); }
//         uart_read_blocking(UART_ID, buf, request->wLength);
//         b = buf;
//         while (len--) {
//           printf("%x ", *b);
//           uart_putc_raw(UART_ID, *(b++));
//         }
//         tud_control_xfer(rhport, request, buf, request->wLength);
//       } else {
//         printf("dir_out\n");
//         uint8_t *b = buf;
//         printf("data: ");
//         while (len--) {
//           printf("%x ", *b);
//           uart_putc_raw(UART_ID, *(b++));
//         }
//       }
//     }
//   } else {
//     return false;
//   }
//   return true;
// }
const uint16_t vid[] = {0x0F0D, 0x12ba, 0x12ba, 0x12ba,
                        0x12ba, ARDWIINO_VID, 0x1bad, 0x1bad};
const uint16_t pid[] = {0x0092, 0x0100, 0x0120, 0x0200,
                        0x0210, ARDWIINO_PID, 0x0004, 0x074B};
uint16_t descriptorRequest(const uint16_t wValue,
                           const uint16_t wIndex,
                           const void **const descriptorAddress) {
    const uint8_t descriptorType = (wValue >> 8);
    const uint8_t descriptorNumber = (wValue & 0xFF);
    uint16_t size = NO_DESCRIPTOR;
    const void *address = NULL;
    switch (descriptorType) {
        case TDTYPE_Device:
            address = &deviceDescriptor;
            size = deviceDescriptor.Header.Size;
            if (deviceType >= SWITCH_GAMEPAD && deviceType < MOUSE) {
                uint8_t offs = (deviceType - SWITCH_GAMEPAD);
                deviceDescriptor.VendorID = vid[offs];
                deviceDescriptor.ProductID = pid[offs];
                // TODO: for the 360, grab this from a passthrough pico
            }
            *descriptorAddress = address;

            return size;
        case TDTYPE_Configuration:
            if (deviceType <= XINPUT_ARCADE_PAD) {
                address = &XBOXConfigurationDescriptor;
                size = XBOXConfigurationDescriptor.Config.TotalConfigurationSize;
                int subType = deviceType;
                if (guitar) {
                    subType = REAL_GUITAR_SUBTYPE;
                } else if (drum) {
                    subType = REAL_DRUM_SUBTYPE;
                }
                XBOXConfigurationDescriptor.Interface1ID.subtype = subType;
            } else if (deviceType <= MOUSE) {
                address = &HIDConfigurationDescriptor;
                size = HIDConfigurationDescriptor.Config.TotalConfigurationSize;
            } else {
                address = &MIDIConfigurationDescriptor;
                size = MIDIConfigurationDescriptor.Config.TotalConfigurationSize;
            }
            // return NO_DESCRIPTOR;
            *descriptorAddress = address;

            return size;
            // break;
        case THID_DTYPE_Report:
            if (deviceType <= KEYBOARD_ROCK_BAND_DRUMS) {
                address = kbd_report_descriptor;
                size = sizeof(kbd_report_descriptor);
            } else {
                address = ps3_report_descriptor;
                size = sizeof(ps3_report_descriptor);
            }
            *descriptorAddress = address;
            return size;
        case TDTYPE_String:
            if (descriptorNumber == 4) {
                address = &xboxString;
            } else if (descriptorNumber < 4) {
                address = descriptorStrings[descriptorNumber];
            } else if (descriptorNumber == 0xEE) {
                address = &OSDescriptorString;
            } else {
                break;
            }
            *descriptorAddress = address;
            size = ((TUSB_StdDescriptor_String_t *)address)->bLength;
            return size;
    }
    return NO_DESCRIPTOR;
}