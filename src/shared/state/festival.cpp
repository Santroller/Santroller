#include <stdint.h>
#include <string.h>

#include "defines.h"
#include "config.h"
#include "reports/controller_reports.h"
#include "state_translation/shared.h"
uint8_t universal_report_to_festival_hid(uint8_t *data, uint8_t sub_type, const USB_Host_Data_t *usb_host_data) {
    uint8_t packet_size = sizeof(PCFortniteRockBandGuitar_Data_t);
    PCFortniteRockBandGuitar_Data_t *report = (PCFortniteRockBandGuitar_Data_t *)data;
    memset(report, 0, sizeof(PCFortniteRockBandGuitar_Data_t));

    report->reportId = GIP_INPUT_REPORT;
    report->green = usb_host_data->green;
    report->red = usb_host_data->red;
    report->yellow = usb_host_data->yellow;
    report->blue = usb_host_data->blue;
    report->orange = usb_host_data->orange;
    report->tilt = (usb_host_data->tilt >> 8) - INT8_MAX;
    if (report->tilt < 0x70) {
        report->tilt = 0;
    }
    report->dpadLeft = usb_host_data->dpadLeft;
    report->dpadRight = usb_host_data->dpadRight;
    report->dpadUp = usb_host_data->dpadUp;
    report->dpadDown = usb_host_data->dpadDown;
    report->guide = usb_host_data->guide;
    report->back = usb_host_data->back;
    report->start = usb_host_data->start;
    //  alias tilt and back to dpad left so that tilt works
    if (report->tilt > 200 || report->back) {
        report->dpadLeft = true;
    }
    // remove back mapping so that back doesn't open menus
    report->back = false;
    report->whammy = (usb_host_data->whammy << 8) - 0x7fff;
    report->pickup = usb_host_data->pickup;
    report->soloGreen = usb_host_data->soloGreen;
    report->soloRed = usb_host_data->soloRed;
    report->soloYellow = usb_host_data->soloYellow;
    report->soloBlue = usb_host_data->soloBlue;
    report->soloOrange = usb_host_data->soloOrange;
    if (usb_host_data->pickup == 0x19) {
        report->pickup = 0x00;
    } else if (usb_host_data->pickup == 0x4C) {
        report->pickup = 0x10;
    } else if (usb_host_data->pickup == 0x96) {
        report->pickup = 0x20;
    } else if (usb_host_data->pickup == 0xB2) {
        report->pickup = 0x30;
    } else if (usb_host_data->pickup == 0xE5) {
        report->pickup = 0x40;
    }
    return packet_size;
}