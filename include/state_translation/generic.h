#include <stdint.h>
#include <string.h>

#include "defines.h"
#include "reports/controller_reports.h"
#include "state_translation/shared.h"
#include "util.h"
extern bool hasFlags;
extern const uint8_t dpad_bindings[11];
extern const uint8_t dpad_bindings_reverse[8];
inline void generic_to_universal_report(const uint8_t *data, uint8_t len, uint8_t sub_type, USB_Host_Data_t *usb_host_data) {
    USB_Host_Data_t *report = (USB_Host_Data_t *)data;
    usb_host_data->genericAxisX = report->genericAxisX;
    usb_host_data->genericAxisY = report->genericAxisY;
    usb_host_data->genericAxisZ = report->genericAxisZ;
    usb_host_data->genericAxisRx = report->genericAxisRx;
    usb_host_data->genericAxisRy = report->genericAxisRy;
    usb_host_data->genericAxisRz = report->genericAxisRz;
    usb_host_data->genericAxisSlider = report->genericAxisSlider;
    usb_host_data->dpadLeft |= report->dpadLeft;
    usb_host_data->dpadRight |= report->dpadRight;
    usb_host_data->dpadUp |= report->dpadUp;
    usb_host_data->dpadDown |= report->dpadDown;
    usb_host_data->genericButtons |= report->genericButtons;
}
// inline void keyboard_to_universal_report(const uint8_t *data, uint8_t len, uint8_t sub_type, USB_Host_Data_t *usb_host_data) {
//     USB_6KRO_Boot_Data_t *report = (USB_6KRO_Boot_Data_t *)data;
//     memcpy(&usb_host_data->keyboard, report, sizeof(USB_6KRO_Boot_Data_t));
// }
inline void keyboard_to_universal_report(const uint8_t *data, uint8_t len, uint8_t sub_type, USB_Host_Data_t *usb_host_data) {
    USB_6KRO_Boot_Data_t *report = (USB_6KRO_Boot_Data_t *)data;
    usb_host_data->keyboard.leftCtrl = report->leftCtrl;
    usb_host_data->keyboard.leftShift = report->leftShift;
    usb_host_data->keyboard.leftAlt = report->leftAlt;
    usb_host_data->keyboard.lWin = report->lWin;
    usb_host_data->keyboard.rightCtrl = report->rightCtrl;
    usb_host_data->keyboard.rightShift = report->rightShift;
    usb_host_data->keyboard.rightAlt = report->rightAlt;
    usb_host_data->keyboard.rWin = report->rWin;
    uint8_t *keyData = usb_host_data->keyboard.raw;
    for (uint8_t i = 0; i < SIMULTANEOUS_KEYS; i++) {
        uint8_t keycode = report->KeyCode[i];
        // F24 is the last supported key in our nkro report
        if (keycode && keycode <= KEYCODE_F24) {
            bit_set(keyData[keycode >> 3], keycode & 7);
        }
    }
}
inline void mouse_to_universal_report(const uint8_t *data, uint8_t len, uint8_t sub_type, USB_Host_Data_t *usb_host_data) {
    USB_Mouse_Boot_Data_t *report = (USB_Mouse_Boot_Data_t *)data;
    memcpy(&usb_host_data->mouse, report, sizeof(USB_Mouse_Boot_Data_t));
}
inline int universal_report_to_keyboard_mouse(uint8_t *data, USB_LastReport_Data_t *last_report, uint8_t len, uint8_t sub_type, const USB_Host_Data_t *usb_host_data) {
    if (sub_type == GUITAR_HERO_GUITAR || sub_type == ROCK_BAND_GUITAR) {
#if KEYBOARD_TYPE == SIXKRO
        uint8_t packet_size = sizeof(USB_6KRO_Data_t);
        USB_6KRO_Data_t *report = (USB_6KRO_Data_t *)data;
        report->rid = REPORT_ID_NKRO;
        uint8_t current = 0;
        if (usb_host_data->green) {
            report->KeyCode[current++] = KEYCODE_1;
        }
        if (usb_host_data->red) {
            report->KeyCode[current++] = KEYCODE_2;
        }
        if (usb_host_data->yellow) {
            report->KeyCode[current++] = KEYCODE_3;
        }
        if (usb_host_data->blue) {
            report->KeyCode[current++] = KEYCODE_4;
        }
        if (usb_host_data->orange) {
            report->KeyCode[current++] = KEYCODE_5;
        }
        if (usb_host_data->dpadDown) {
            report->KeyCode[current++] = KEYCODE_ENTER;
        }
        if (usb_host_data->back && current < SIMULTANEOUS_KEYS) {
            report->KeyCode[current++] = KEYCODE_SPACE;
        }
        report->rightShift = usb_host_data->dpadUp;
        uint8_t cmp = memcmp(&last_report->last6KROReport, data, packet_size);
        if (cmp == 0) {
            return 0;
        }
        return packet_size;
#else
        USB_NKRO_Data_t *report = (USB_NKRO_Data_t *)data;
        uint8_t packet_size = sizeof(USB_NKRO_Data_t);
        report->rid = REPORT_ID_NKRO;
        report->d1 = usb_host_data->green;
        report->d2 = usb_host_data->red;
        report->d3 = usb_host_data->yellow;
        report->d4 = usb_host_data->blue;
        report->d5 = usb_host_data->orange;
        report->rightShift = usb_host_data->dpadUp;
        report->enter = usb_host_data->dpadDown;
        report->space = usb_host_data->back;
        uint8_t cmp = memcmp(&last_report->lastNKROReport, data, packet_size);
        if (cmp == 0) {
            return 0;
        }
        return packet_size;
#endif
    }
    uint8_t packet_size = 0;
    void *lastReportToCheck;
    for (int i = 1; i < REPORT_ID_END; i++) {
#ifdef HAS_MOUSE
        if (i == REPORT_ID_MOUSE) {
            packet_size = sizeof(USB_Mouse_Data_t);
            memset(data, 0, packet_size);
            USB_Mouse_Data_t *report = (USB_Mouse_Data_t *)data;
            memcpy(data, &usb_host_data->mouse, sizeof(USB_Mouse_Data_t));
            report->rid = REPORT_ID_MOUSE;
            if (last_report) {
                lastReportToCheck = &last_report->lastMouseReport;
            }
        }
#endif
#if KEYBOARD_TYPE == NKRO
        if (i == REPORT_ID_CONSUMER) {
            packet_size = sizeof(USB_ConsumerControl_Data_t);
            memset(data, 0, packet_size);
            memcpy(data, &usb_host_data->consumerControl, sizeof(USB_ConsumerControl_Data_t));
            USB_ConsumerControl_Data_t *report = (USB_ConsumerControl_Data_t *)data;
            report->rid = REPORT_ID_CONSUMER;
            if (last_report) {
                lastReportToCheck = &last_report->lastConsumerReport;
            }
        }
        if (i == REPORT_ID_NKRO) {
            packet_size = sizeof(USB_NKRO_Data_t);
            memset(data, 0, packet_size);
            USB_NKRO_Data_t *report = (USB_NKRO_Data_t *)data;
            report->rid = REPORT_ID_NKRO;
            memcpy(data, &usb_host_data->keyboard, sizeof(USB_NKRO_Data_t));
            if (last_report) {
                lastReportToCheck = &last_report->lastNKROReport;
            }
        }
#elif KEYBOARD_TYPE == SIXKRO
        if (i == REPORT_ID_NKRO) {
            packet_size = sizeof(USB_6KRO_Data_t);
            memset(data, 0, packet_size);
            USB_6KRO_Data_t *report = (USB_6KRO_Data_t *)data;
            report->rid = REPORT_ID_NKRO;
            report->leftCtrl = usb_host_data->keyboard.leftCtrl;
            report->leftShift = usb_host_data->keyboard.leftShift;
            report->leftAlt = usb_host_data->keyboard.leftAlt;
            report->lWin = usb_host_data->keyboard.lWin;
            report->rightCtrl = usb_host_data->keyboard.rightCtrl;
            report->rightShift = usb_host_data->keyboard.rightShift;
            report->rightAlt = usb_host_data->keyboard.rightAlt;
            report->rWin = usb_host_data->keyboard.rWin;
            const uint8_t *keyData = usb_host_data->keyboard.raw;
            uint8_t set = 0;
            for (uint8_t keycode = 0; i < sizeof(usb_host_data->keyboard.raw) << 3 && set < SIMULTANEOUS_KEYS; keycode++) {
                if (keyData[keycode >> 3] & (keycode & 7)) {
                    report->KeyCode[set++] = keycode;
                }
            }
            if (last_report) {
                lastReportToCheck = &last_report->last6KROReport;
            }
        }
#endif

        // If we are directly asked for a HID report, always just reply with the NKRO one
        if (lastReportToCheck) {
            uint8_t cmp = memcmp(lastReportToCheck, data, packet_size);
            if (cmp == 0) {
                packet_size = 0;
                continue;
            }
            memcpy(lastReportToCheck, data, packet_size);
            break;
        } else {
            break;
        }
    }
    if (packet_size) {
        return packet_size;
    }
}