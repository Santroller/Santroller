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
#include "xinput_device.h"
int validAnalog = 0;

bool isRF = false;
bool typeIsGuitar;
bool typeIsDrum;
uint8_t inputType;
uint8_t pollRate;

#define UART_ID uart1
#define BAUD_RATE 115200

// TODO: how big does this need to be?
CFG_TUSB_MEM_SECTION CFG_TUSB_MEM_ALIGN uint8_t buf[255];
// We are using pins 4 and 5, but see the GPIO function select table in the
// datasheet for information on which other pins can be used.
#define UART_TX_PIN 4
#define UART_RX_PIN 5
bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request) {
    uint8_t *data;
    bool valid = false;
    if (request->bmRequestType_bit.direction == USB_DIR_DEVICE_TO_HOST) {
        if (stage == CONTROL_STAGE_SETUP) {
            requestType_t r = {bmRequestType : request->bmRequestType};
            uint16_t len = controlRequest(r, request->bRequest, request->wValue, request->wIndex, request->wLength, &data, &valid);
            if (valid) {
                tud_control_xfer(rhport, request, data, len);
            }
        }
    } else {
        if (stage == CONTROL_STAGE_SETUP) {
            tud_control_xfer(rhport, request, buf, request->wLength);
        } else if (stage == CONTROL_STAGE_ACK) {
            data = buf;
            requestType_t r = {bmRequestType : request->bmRequestType};
            controlRequest(r, request->bRequest, request->wValue, request->wIndex, request->wLength, &data, &valid);
        }
    }
}

uint8_t const *tud_descriptor_device_cb(void) {
    descriptorRequest(TDTYPE_Device << 8, 0, buf);
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
    descriptorRequest(TDTYPE_String << 8 | index, 0, buf);
    return buf;
}

int main() {
    generateSerialString(serialString.UnicodeString);
    init();
    board_init();
    tusb_init();
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    while (1) {
        tud_task();  // tinyusb device task
        if (consoleType == XBOX360) {
            if (tud_xinput_n_ready(0)) {
                tud_xinput_n_report(0, 0, data, size);
            }
        } else if (consoleType == MIDI) {
            if (tud_hid_n_ready(0)) {
                tud_hid_n_report(0, rid, data, size);
            }
        } else {
            tud_midi_n_packet_write(0, data);
        }
    }
}

usbd_class_driver_t driver[] = {{.init = xinputd_init,
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
    uint8_t valid;
    return controlRequest(r, THID_REQ_GetReport, report_type << 8 | report_id, instance, reqlen, &buffer, &valid);
}
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id,
                           hid_report_type_t report_type, uint8_t const *buffer,
                           uint16_t bufsize) {
    requestType_t r = {bmRequestType_bit : {direction : USB_DIR_HOST_TO_DEVICE, recipient : USB_REQ_RCPT_INTERFACE, type : USB_REQ_TYPE_CLASS}};
    uint8_t valid;
    controlRequest(r, THID_REQ_SetReport, report_type << 8 | report_id, instance, bufsize, &buffer, &valid);
}