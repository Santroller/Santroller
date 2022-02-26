#include "usb.h"

#include <stddef.h>
#include <stdio.h>

#include "defines.h"
#include "descriptors.h"
#include "lib_main.h"
#include "report_descriptors.h"
#include "string.h"
#include "usb/ps3_descriptors.h"
#include "usb/xinput_descriptors.h"
uint16_t controlRequest(const requestType_t requestType, const uint8_t request, const uint16_t wValue, const uint16_t wIndex, const uint16_t wLength, void *requestBuffer, bool *valid) {
    *valid = true;
    if (requestType.bmRequestType_bit.direction == USB_DIR_DEVICE_TO_HOST) {
        if (requestType.bmRequestType_bit.type == USB_REQ_TYPE_VENDOR) {
            if (requestType.bmRequestType_bit.recipient == USB_REQ_RCPT_INTERFACE) {
                if (request == THID_REQ_GetReport && wIndex == INTERFACE_ID_Device && wValue == 0x0000) {
                    memcpy_P(requestBuffer, capabilities1, sizeof(capabilities1));
                    return sizeof(capabilities1);
                } else if (request == REQ_GET_OS_FEATURE_DESCRIPTOR && wIndex == DESC_EXTENDED_PROPERTIES_DESCRIPTOR && wValue == INTERFACE_ID_Config) {
                    memcpy_P(requestBuffer, &ExtendedIDs, ExtendedIDs.TotalLength);
                    return ExtendedIDs.TotalLength;
                } else if (request == THID_REQ_GetReport && wIndex == INTERFACE_ID_Device && wValue == 0x0100) {
                    memcpy_P(requestBuffer, capabilities2, sizeof(capabilities2));
                    return sizeof(capabilities2);
                }
            } else if (requestType.bmRequestType_bit.recipient == USB_REQ_RCPT_DEVICE) {
                if (request == REQ_GET_OS_FEATURE_DESCRIPTOR && wIndex == DESC_EXTENDED_COMPATIBLE_ID_DESCRIPTOR) {
                    memcpy_P(requestBuffer, &DevCompatIDs, DevCompatIDs.TotalLength);
                    if (consoleType == PC_XINPUT) {
                        TUSB_OSCompatibleIDDescriptor_t* compat = (TUSB_OSCompatibleIDDescriptor_t*)requestBuffer;
                        compat->TotalSections = 2;
                    }
                    return DevCompatIDs.TotalLength;
                } else if (request == THID_REQ_GetReport && wIndex == 0x00 && wValue == 0x0000) {
                    memcpy_P(requestBuffer, ID, sizeof(ID));
                    return sizeof(ID);
                }
            }
        } else if (request == THID_REQ_GetReport && requestType.bmRequestType_bit.type == USB_REQ_TYPE_CLASS && requestType.bmRequestType_bit.recipient == USB_REQ_RCPT_INTERFACE && wIndex == 0x0300) {
            if (consoleType == PS3) {
                memcpy_P(requestBuffer, ps3_init, sizeof(ps3_init));
                if (deviceType <= ROCK_BAND_DRUMS) {
                    ((uint8_t *)requestBuffer)[3] = 0x00;
                } else if (deviceType <= GUITAR_HERO_DRUMS) {
                    ((uint8_t *)requestBuffer)[3] = 0x06;
                }
                return sizeof(ps3_init);
            } else if (consoleType == PC) {
                consoleType = PS3;
                reset_usb();
            }
        } else if (request == THID_REQ_GetReport && requestType.bmRequestType_bit.type == USB_REQ_TYPE_CLASS && requestType.bmRequestType_bit.recipient == USB_REQ_RCPT_INTERFACE && wIndex == 0x0300) {
            if (consoleType == PS3) {
                memcpy_P(requestBuffer, ps3_init, sizeof(ps3_init));
                if (deviceType <= ROCK_BAND_DRUMS) {
                    ((uint8_t *)requestBuffer)[3] = 0x00;
                } else if (deviceType <= GUITAR_HERO_DRUMS) {
                    ((uint8_t *)requestBuffer)[3] = 0x06;
                }
                return sizeof(ps3_init);
            } else if (consoleType == PC) {
                consoleType = PS3;
                reset_usb();
            }
        }
    } else {
        // uint8_t *data = *address;
        if (request == THID_REQ_SetReport && requestType.bmRequestType_bit.type == USB_REQ_TYPE_CLASS && requestType.bmRequestType_bit.recipient == USB_REQ_RCPT_INTERFACE) {
            
        } else if (consoleType == PC && request == USB_REQ_CLEAR_FEATURE && requestType.bmRequestType_bit.type == USB_REQ_TYPE_STANDARD && requestType.bmRequestType_bit.recipient == USB_REQ_RCPT_ENDPOINT && wIndex == DEVICE_EPADDR_OUT) {
            consoleType = SWITCH;
            reset_usb();
        } else if (consoleType == PC && request == USB_REQ_CLEAR_FEATURE && requestType.bmRequestType_bit.type == USB_REQ_TYPE_STANDARD && requestType.bmRequestType_bit.recipient == USB_REQ_RCPT_ENDPOINT && wIndex == DEVICE_EPADDR_IN) {
            consoleType = SWITCH;
            reset_usb();
        }
    }
    *valid = false;
    return 0;
}
const uint16_t vid[] = {0x0F0D, 0x12ba, 0x12ba, 0x12ba,
                        0x12ba, ARDWIINO_VID, 0x1bad, 0x1bad};
const uint16_t pid[] = {0x0092, 0x0100, 0x0120, 0x0200,
                        0x0210, ARDWIINO_PID, 0x0004, 0x074B};
uint16_t descriptorRequest(const uint16_t wValue,
                           const uint16_t wIndex,
                           void *descriptorBuffer) {
    const uint8_t descriptorType = (wValue >> 8);
    const uint8_t descriptorNumber = (wValue & 0xFF);
    // printf("Type: %d, num: %d\n", descriptorType, descriptorNumber);
    // printf("CT: %d, DT: %d\n", consoleType, deviceType);
    uint16_t size = NO_DESCRIPTOR;
    switch (descriptorType) {
        case TDTYPE_Device:
            size = sizeof(TUSB_Descriptor_Device_t);
            memcpy_P(descriptorBuffer, &deviceDescriptor, size);
            TUSB_Descriptor_Device_t *dev = (TUSB_Descriptor_Device_t *)descriptorBuffer;
            if (consoleType == PS3 || consoleType == PC) {
                if (deviceType > GUITAR_HERO_GUITAR) {
                    dev->VendorID = SONY_VID;
                    switch (deviceType) {
                        case GUITAR_HERO_DRUMS:
                            dev->ProductID = PS3_GH_DRUM_PID;
                            break;
                        case GUITAR_HERO_GUITAR:
                            dev->ProductID = PS3_GH_GUITAR_PID;
                            break;
                        case ROCK_BAND_GUITAR:
                            dev->ProductID = PS3_RB_GUITAR_PID;
                            break;
                        case ROCK_BAND_DRUMS:
                            dev->ProductID = PS3_RB_DRUM_PID;
                            break;
                        case GUITAR_HERO_LIVE_GUITAR:
                            dev->ProductID = PS3WIIU_GHLIVE_DONGLE_PID;
                            break;
                        case DJ_HERO_TURNTABLE:
                            dev->ProductID = PS3_DJ_TURNTABLE_PID;
                            break;
                        default:
                            break;
                    }
                }
            } else if (consoleType == SWITCH) {
                dev->VendorID = HORI_VID;
                dev->ProductID = HORI_POKKEN_TOURNAMENT_DX_PRO_PAD_PID;
            } else if (consoleType == WII_RB) {
                dev->VendorID = WII_RB_VID;
                if (drum) {
                    dev->ProductID = WII_RB_DRUM_PID;
                } else {
                    dev->ProductID = WII_RB_GUITAR_PID;
                }
            }
            // for (int i = 0; i < size; i++) {
            //     printf("0x%x, ", ((uint8_t*)descriptorBuffer)[i]);
            // }
            // printf("\n");
            break;
        case TDTYPE_Configuration:
            if (consoleType == XBOX360 || consoleType == PC_XINPUT) {
                size = sizeof(TUSB_Descriptor_Configuration_XBOX_t);
                memcpy_P(descriptorBuffer, &XBOXConfigurationDescriptor, size);
                TUSB_Descriptor_Configuration_XBOX_t *desc = (TUSB_Descriptor_Configuration_XBOX_t *)descriptorBuffer;
                SubType_t subType;
                switch (deviceType) {
                    case GAMEPAD:
                        subType = XINPUT_GAMEPAD;
                        break;
                    case WHEEL:
                        subType = XINPUT_WHEEL;
                        break;
                    case ARCADE_STICK:
                        subType = XINPUT_ARCADE_STICK;
                        break;
                    case GUITAR_HERO_GUITAR:
                    case ROCK_BAND_GUITAR:
                        subType = XINPUT_GUITAR_ALTERNATE;
                        break;
                    case GUITAR_HERO_LIVE_GUITAR:
                        subType = XINPUT_LIVE_GUITAR;
                        break;
                    case GUITAR_HERO_DRUMS:
                    case ROCK_BAND_DRUMS:
                        subType = XINPUT_DRUMS;
                        break;
                    case FLIGHT_STICK:
                        subType = XINPUT_FLIGHT_STICK;
                        break;
                    case DANCE_PAD:
                        subType = XINPUT_DANCE_PAD;
                        break;
                    case ARCADE_PAD:
                        subType = XINPUT_ARCADE_PAD;
                        break;
                    case DJ_HERO_TURNTABLE:
                        subType = XINPUT_TURNTABLE;
                        break;
                }
                desc->Interface1ID.subtype = subType;
            } else if (consoleType == MIDI) {
                size = sizeof(TUSB_Descriptor_MIDI_Configuration_t);
                memcpy_P(descriptorBuffer, &MIDIConfigurationDescriptor, size);
            } else {
                size = sizeof(TUSB_Descriptor_HID_Configuration_t);
                memcpy_P(descriptorBuffer, &HIDConfigurationDescriptor, size);
                TUSB_Descriptor_HID_Configuration_t *desc = (TUSB_Descriptor_HID_Configuration_t *)descriptorBuffer;
                if (consoleType == WII_RB) {
                    desc->Config.TotalInterfaces = 1;
                    desc->Config.TotalConfigurationSize = offsetof(TUSB_Descriptor_HID_Configuration_t, InterfaceConfig);
                }
                if (consoleType == KEYBOARD_MOUSE) {
                    desc->HIDDescriptor.HIDReportLength = sizeof(kbd_report_descriptor);
                } else if (consoleType == PS3 || consoleType == WII_RB || consoleType == SWITCH || consoleType == PC) {
                    desc->HIDDescriptor.HIDReportLength = sizeof(ps3_report_descriptor);
                }
            }
            break;
        case THID_DTYPE_Report:
            read_hid_report_descriptor = true;
            const void *address;
            if (consoleType == KEYBOARD_MOUSE) {
                address = kbd_report_descriptor;
                size = sizeof(kbd_report_descriptor);
            } else if (consoleType == PS3 || consoleType == WII_RB || consoleType == SWITCH || consoleType == PC) {
                address = ps3_report_descriptor;
                size = sizeof(ps3_report_descriptor);
            }
            memcpy_P(descriptorBuffer, address, size);
            break;
        case TDTYPE_String:
            const void *str;
            if (descriptorNumber == 4) {
                str = &xboxString;
            } else if (descriptorNumber < 4) {
#ifdef __AVR__
                str = (void *)pgm_read_word(descriptorStrings + descriptorNumber);
#else
                str = descriptorStrings[descriptorNumber];
#endif
            } else if (descriptorNumber == 0xEE) {
                str = &OSDescriptorString;
            } else {
                break;
            }
#ifdef __AVR__
            size = pgm_read_byte(str + offsetof(TUSB_Descriptor_Header_t, Size));
#else
            size = ((TUSB_Descriptor_Header_t *)str)->Size;
#endif
            memcpy_P(descriptorBuffer, str, size);
            break;
    }
    return size;
}