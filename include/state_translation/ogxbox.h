#include <stdint.h>

#include "defines.h"
#include "reports/controller_reports.h"
#include "state_translation/generic.h"
extern bool hasFlags;
extern const uint8_t dpad_bindings[11];
extern const uint8_t dpad_bindings_reverse[8];
inline void ogxbox_to_universal_report(const uint8_t *data, uint8_t len, uint8_t sub_type, USB_Host_Data_t *usb_host_data) {
    OGXboxGamepad_Data_t *report = (OGXboxGamepad_Data_t *)data;
    usb_host_data->dpadLeft |= report->dpadLeft;
    usb_host_data->dpadRight |= report->dpadRight;
    usb_host_data->dpadUp |= report->dpadUp;
    usb_host_data->dpadDown |= report->dpadDown;
    usb_host_data->start |= report->start;
    usb_host_data->back |= report->back;
    usb_host_data->a |= report->a;
    usb_host_data->b |= report->b;
    usb_host_data->x |= report->x;
    usb_host_data->y |= report->y;
    usb_host_data->leftShoulder |= report->leftShoulder;
    usb_host_data->rightShoulder |= report->rightShoulder;
    if (report->leftTrigger) {
        usb_host_data->leftTrigger = report->leftTrigger << 8;
    }
    if (report->rightTrigger) {
        usb_host_data->rightTrigger = report->rightTrigger << 8;
    }
    if (report->leftStickX != 0x80) {
        usb_host_data->leftStickX = report->leftStickX;
    }
    if (report->leftStickY != 0x80) {
        usb_host_data->leftStickY = -report->leftStickY;
    }
    if (report->rightStickX != 0x80) {
        usb_host_data->rightStickX = report->rightStickX;
    }
    if (report->rightStickY != 0x80) {
        usb_host_data->rightStickY = -report->rightStickY;
    }
    usb_host_data->leftThumbClick |= report->leftThumbClick;
    usb_host_data->rightThumbClick |= report->rightThumbClick;
    ;
}

inline void universal_report_to_ogxbox(uint8_t *data, uint8_t len, uint8_t sub_type, const USB_Host_Data_t *usb_host_data) {
    OGXboxGamepad_Data_t *report = (OGXboxGamepad_Data_t *)data;
    report->dpadLeft = usb_host_data->dpadLeft;
    report->dpadRight = usb_host_data->dpadRight;
    report->dpadUp = usb_host_data->dpadUp;
    report->dpadDown = usb_host_data->dpadDown;
    report->back |= usb_host_data->back;
    report->start |= usb_host_data->start;
    report->a |= usb_host_data->a;
    report->b |= usb_host_data->b;
    report->x |= usb_host_data->x;
    report->y |= usb_host_data->y;
    report->leftShoulder |= usb_host_data->leftShoulder;
    report->rightShoulder |= usb_host_data->rightShoulder;
    if (usb_host_data->leftTrigger) {
        report->leftTrigger = usb_host_data->leftTrigger >> 8;
    }
    if (usb_host_data->rightTrigger) {
        report->rightTrigger = usb_host_data->rightTrigger >> 8;
    }
    if (usb_host_data->leftStickX != 0x80) {
        report->leftStickX = usb_host_data->leftStickX;
    }
    if (usb_host_data->leftStickY != 0x80) {
        report->leftStickY = -usb_host_data->leftStickY;
    }
    if (usb_host_data->rightStickX != 0x80) {
        report->rightStickX = usb_host_data->rightStickX;
    }
    if (usb_host_data->rightStickY != 0x80) {
        report->rightStickY = -usb_host_data->rightStickY;
    }
    report->leftThumbClick |= usb_host_data->leftThumbClick;
    report->rightThumbClick |= usb_host_data->rightThumbClick;
    ;
}