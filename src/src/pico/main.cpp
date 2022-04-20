#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <pico/stdlib.h>
#include <pico/unique_id.h>
#include "pico/multicore.h"

#include "config.h"
#include "device/usbd_pvt.h"
#include "hid/hid_device.h"
#include "pins.h"
extern "C" {
	#include "common/tusb_types.h"
	#include "pio_usb.h"
	#include <tusb.h>
}
#include "serial.h"
#include "xinput_device.h"


static usb_device_t *usb_device = NULL;
static usb_device_t *xinput_device = NULL;
static uint8_t xinput_endpoint = 0;
static uint8_t xinput_endpoint_in = 0;
CFG_TUSB_MEM_SECTION CFG_TUSB_MEM_ALIGN uint8_t buf[255];
CFG_TUSB_MEM_SECTION CFG_TUSB_MEM_ALIGN uint8_t buf2[255];
void core1_main() {
    // To run USB SOF interrupt in core1, create alarm pool in core1.
    static pio_usb_configuration_t config = PIO_USB_DEFAULT_CONFIG;
    config.pin_dp = PIN_USB_DP_PIN;
    config.alarm_pool = (void *)alarm_pool_create(2, 1);
    usb_device = pio_usb_host_init(&config);

    while (true) {
        pio_usb_host_task();
    }
}


void set_xinput_led(usb_device_t *device, uint endpoint, uint8_t led) {
    uint8_t data2[] = {0x01, 0x03, led};
    endpoint_t *ep = pio_usb_get_endpoint(device, endpoint);
    pio_usb_set_out_data(ep, data2, sizeof(data2));
}
void host_connection(usb_device_t *device) {
    // Print received packet to EPs
    for (int ep_idx = 0; ep_idx < PIO_USB_DEV_EP_CNT; ep_idx++) {
        endpoint_t *ep = pio_usb_get_endpoint(device, ep_idx);

        if (ep == NULL) {
            break;
        }
        if (ep->dev_class == 0xFF && ep->protocol == 0x01 && ep->sub_class == 0x5d && !(ep->ep_num & EP_IN)) {
            set_xinput_led(device, ep_idx, 0x0A);
            if (!xinput_device) {
                xinput_device = device;
                xinput_endpoint = ep_idx;
                tud_disconnect();
                sleep_ms(1000);
                tud_connect();
            }
            return;
        } else if (ep->dev_class == 0xFF && ep->protocol == 0x01 && ep->sub_class == 0x5d && (ep->ep_num & EP_IN)) {
            if (!xinput_device || xinput_device == device) {
                xinput_endpoint_in = ep_idx;
            }
        }
    }
}
void host_disconnection(usb_device_t *device) {
    for (int ep_idx = 0; ep_idx < PIO_USB_DEV_EP_CNT; ep_idx++) {
        endpoint_t *ep = pio_usb_get_endpoint(device, ep_idx);

        if (ep == NULL) {
            break;
        }

        if (ep->dev_class == 0xFF && ep->protocol == 0x01 && ep->sub_class == 0x5d) {
            if (xinput_device == device) {
                xinput_device = NULL;
            }
            return;
        }
    }
}
void setup() {
	uart_set_baudrate(uart0, 115200);
    generateSerialString(serialString.UnicodeString);
    multicore_reset_core1();
    multicore_launch_core1(core1_main);
    tusb_init();
    set_device_connection_handler(&host_connection);
    set_device_disconnection_handler(&host_disconnection);
}

void loop() {
    tud_task();  // tinyusb device task
    pio_usb_connection_task();
    // uint8_t len = 20;
    // if (xinput_device) {
    //     pio_usb_get_in_data(pio_usb_get_endpoint(xinput_device, xinput_endpoint_in), controller, 20);
    // } else {
    //     len = tick(controller);
    // }
    // if (consoleType == XBOX360) {
    //     if (tud_xinput_n_ready(0)) {
    //         tud_xinput_n_report(0, 0, controller, len);
    //     }
    // } else if (consoleType == MIDI) {
    //     tud_midi_n_packet_write(0, controller);
    // } else {
    //     if (tud_hid_custom_n_ready(0)) {
    //         tud_hid_custom_n_report(0, 0, controller, len);
    //     }
    // }
}

uint16_t tud_hid_custom_get_report_cb(uint8_t instance, uint8_t report_id,
                                      hid_report_type_t report_type, uint8_t *buffer,
                                      uint16_t reqlen) {
    // requestType_t r = {bmRequestType_bit : {direction : USB_DIR_DEVICE_TO_HOST, recipient : USB_REQ_RCPT_INTERFACE, type : USB_REQ_TYPE_CLASS}};
    // bool valid;
    // return controlRequest(r, THID_REQ_GetReport, instance, report_type << 8 | report_id, reqlen, &buffer, &valid);
    return 0;
}
void tud_hid_custom_set_report_cb(uint8_t instance, uint8_t report_id,
                                  hid_report_type_t report_type, uint8_t const *buffer,
                                  uint16_t bufsize) {
    //     requestType_t r = {bmRequestType_bit : {direction : USB_DIR_HOST_TO_DEVICE, recipient : USB_REQ_RCPT_INTERFACE, type : USB_REQ_TYPE_CLASS}};
    //     bool valid;
    //     controlRequest(r, THID_REQ_SetReport, instance, report_type << 8 | report_id, bufsize, &buffer, &valid);
}

uint8_t const *tud_descriptor_device_cb(void) {
    descriptorRequest(USB_DESCRIPTOR_DEVICE << 8, 0, buf);
    if (usb_device->enumerated && consoleType == XBOX360) {
        USB_DEVICE_DESCRIPTOR *td = (USB_DEVICE_DESCRIPTOR *)buf;
        td->idVendor = usb_device->vid;
        td->idProduct = usb_device->pid;
    }
    return buf;
}
uint8_t const *tud_hid_custom_descriptor_report_cb(uint8_t instance) {
    descriptorRequest(HID_DESCRIPTOR_REPORT << 8, 0, buf);
    return buf;
}
uint8_t const *tud_descriptor_configuration_cb(uint8_t index) {
    (void)index;  // for multiple configurations
    descriptorRequest(USB_DESCRIPTOR_CONFIGURATION << 8, 0, buf);
    return buf;
}
uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    // Arduinos handle their own serial descriptor, so theres no point in sharing an implementation there.
    if (index == 3) {
        return (uint16_t *)&serialString;
    }
    if (descriptorRequest(USB_DESCRIPTOR_STRING << 8 | index, 0, buf)) {
        return (uint16_t *)buf;
    }
    return NULL;
}

void reset_usb(void) {
    tud_disconnect();
    sleep_ms(1000);
    tud_connect();
}
tusb_control_request_t lastreq;
bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request) {
    if (consoleType != XBOX360 && request->bmRequestType == 0xC1 && request->bRequest == 0x81) {
        consoleType = XBOX360;
        printf("XBOX detected!\n");
        if (xinput_device) {
            reset_usb();
            return 0;
        }
    }
    bool valid = false;
    usb_setup_packet_t req_host = {
        USB_SYNC, USB_PID_DATA0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    req_host.crc16[0] = USB_CRC16_PLACE;
    req_host.crc16[1] = USB_CRC16_PLACE;
    memcpy(&req_host.request_type, request, sizeof(tusb_control_request_t));
    update_packet_crc16(&req_host);
    if (request->bmRequestType_bit.direction == TUSB_DIR_IN) {
        if (stage == CONTROL_STAGE_SETUP) {
            uint16_t len = controlRequest(request->bmRequestType, request->bRequest, request->wValue, request->wIndex, request->wLength, buf, &valid);
            if (len > request->wLength || !valid) len = request->wLength;
            if (!valid && xinput_device) {
                if (control_in_protocol(xinput_device, (uint8_t*)&req_host, sizeof(req_host), buf, len) < 0) {
                    return false;
                }
            }
            tud_control_xfer(rhport, request, buf, len);
        }
    } else {
        if (stage == CONTROL_STAGE_SETUP) {
            tud_control_xfer(rhport, request, buf, request->wLength);
        }
        if (stage == CONTROL_STAGE_DATA || (stage == CONTROL_STAGE_SETUP && !request->wLength)) {
            controlRequest(request->bmRequestType, request->bRequest, request->wValue, request->wIndex, request->wLength, buf, &valid);
            if (!valid && xinput_device) {
                if (control_out_protocol(xinput_device, (uint8_t*)&req_host, sizeof(req_host), request->wLength ? buf : NULL, request->wLength) < 0) {
                    return false;
                }
            }
        }
    }
    return true;
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
                                    .sof = NULL},
                                {
#if CFG_TUSB_DEBUG >= 2
                                    .name = "HID",
#endif
                                    .init = hidd_custom_init,
                                    .reset = hidd_custom_reset,
                                    .open = hidd_custom_open,
                                    .control_xfer_cb = hidd_custom_control_xfer_cb,
                                    .xfer_cb = hidd_custom_xfer_cb,
                                    .sof = NULL}};

usbd_class_driver_t const *usbd_app_driver_get_cb(uint8_t *driver_count) {
    *driver_count = 2;
    return driver;
}