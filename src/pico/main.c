#include <device/usbd_pvt.h>
#include <hardware/sync.h>
#include <pico/stdlib.h>
#include <pico/types.h>
#include <pico/unique_id.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <tusb.h>

#include "bsp/board.h"
#include "defines.h"
#include "descriptors.h"
#include "lib_main.h"
#include "serial.h"
#include "stdbool.h"
#include "usb.h"
#include "usb_host/usb_host.h"
#include "xinput_device.h"

#define UART_ID uart1
#define BAUD_RATE 115200
uint8_t controller[DEVICE_EPSIZE_IN];

// TODO: how big does this need to be?
CFG_TUSB_MEM_SECTION CFG_TUSB_MEM_ALIGN uint8_t buf[255];
CFG_TUSB_MEM_SECTION CFG_TUSB_MEM_ALIGN uint8_t buf2[255];
// We are using pins 4 and 5, but see the GPIO function select table in the
// datasheet for information on which other pins can be used.
#define UART_TX_PIN 4
#define UART_RX_PIN 5
tusb_control_request_t lastreq;

bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request) {
    uint8_t *data;
    bool valid = false;
    if (request->bmRequestType_bit.direction == USB_DIR_DEVICE_TO_HOST) {
        if (stage == CONTROL_STAGE_SETUP) {
            requestType_t r = {bmRequestType : request->bmRequestType};
            uint16_t len = controlRequest(r, request->bRequest, request->wValue, request->wIndex, request->wLength, &data, &valid);
            if (!valid) {
                if (lastreq.bmRequestType == request->bmRequestType && lastreq.bRequest == request->bRequest && request->bRequest != 0x86 && buf2[request->wLength - 1]) {
                    data = buf2;
                } else {
                    data = buf;
                    memset(buf, 0, sizeof(buf));
                    send_control_request(13, 0, *request, false, data);
                    memcpy(buf2, buf, sizeof(buf));
                }
                len = request->wLength;
            }
            tud_control_xfer(rhport, request, data, len);
            if (data != buf2) {
                printf("Ctrl tx: %d, %x %x, cached=%d\n", valid, request->wIndex, request->wValue, data == buf2);
                for (int i = 0; i < request->wLength; i++) {
                    printf("0x%x, ", data[i]);
                }
                printf("\n");
            }
            lastreq = *request;
        }
    } else {
        if (stage == CONTROL_STAGE_SETUP) {
            tud_control_xfer(rhport, request, buf, request->wLength);
        } else if (stage == CONTROL_STAGE_ACK) {
            data = buf;
            requestType_t r = {bmRequestType : request->bmRequestType};
            controlRequest(r, request->bRequest, request->wValue, request->wIndex, request->wLength, &data, &valid);
            if (!valid) {
                send_control_request(13, 0, *request, false, data);
            }
            printf("Ctrl rx: %d, %x %x\n", valid, request->wIndex, request->wValue);
        }
    }
}

uint8_t const *tud_descriptor_device_cb(void) {
    descriptorRequest(TDTYPE_Device << 8, 0, buf);
    TUSB_Descriptor_Device_t *td = (TUSB_Descriptor_Device_t *)buf;
    TUSB_Descriptor_Device_t host = getHostDescriptor();
    td->VendorID = host.VendorID;
    td->ProductID = host.ProductID;
    return buf;
}
uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance) {
    descriptorRequest(THID_DTYPE_Report << 8, 0, buf);
    return buf;
}
uint8_t const *tud_descriptor_configuration_cb(uint8_t index) {
    (void)index;  // for multiple configurations
    descriptorRequest(TDTYPE_Configuration << 8, 0, buf);
    return buf;
}
uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    // Arduinos handle their own serial descriptor, so theres no point in sharing an implementation there.
    if (index == 3) {
        return (uint16_t *)&serialString;
    }
    if (descriptorRequest(TDTYPE_String << 8 | index, 0, buf)) {
        return (uint16_t *)buf;
    }
    return NULL;
}

int main() {
    set_sys_clock_khz(133000, true);
    generateSerialString(serialString.UnicodeString);
    board_init();
    tusb_init();
    init();
    init_usb_host();
    while (1) {
        tick_usb_host();
        tud_task();  // tinyusb device task
        uint8_t len = tick(controller);
        if (consoleType == XBOX360) {
            if (tud_xinput_n_ready(0)) {
                tud_xinput_n_report(0, 0, controller, len);
            }
        } else if (consoleType == MIDI) {
            tud_midi_n_packet_write(0, controller);
        } else {
            if (tud_hid_n_ready(0)) {
                tud_hid_n_report(0, 0, controller, len);
            }
        }
    }
}

usbd_class_driver_t driver[] = {{
#if CFG_TUSB_DEBUG >= 2
    .name = "XInput",
#endif
    .init = xinputd_init,
    .reset = xinputd_reset,
    .open = xinputd_open,
    .control_xfer_cb = tud_vendor_control_xfer_cb,
    .xfer_cb = xinputd_xfer_cb,
    .sof = NULL}};

usbd_class_driver_t const *usbd_app_driver_get_cb(uint8_t *driver_count) {
    *driver_count = 1;
    return driver;
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id,
                               hid_report_type_t report_type, uint8_t *buffer,
                               uint16_t reqlen) {
    requestType_t r = {bmRequestType_bit : {direction : USB_DIR_DEVICE_TO_HOST, recipient : USB_REQ_RCPT_INTERFACE, type : USB_REQ_TYPE_CLASS}};
    bool valid;
    return controlRequest(r, THID_REQ_GetReport, report_type << 8 | report_id, instance, reqlen, &buffer, &valid);
}
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id,
                           hid_report_type_t report_type, uint8_t const *buffer,
                           uint16_t bufsize) {
    requestType_t r = {bmRequestType_bit : {direction : USB_DIR_HOST_TO_DEVICE, recipient : USB_REQ_RCPT_INTERFACE, type : USB_REQ_TYPE_CLASS}};
    bool valid;
    controlRequest(r, THID_REQ_SetReport, report_type << 8 | report_id, instance, bufsize, &buffer, &valid);
}