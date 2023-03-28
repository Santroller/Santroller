#include <Arduino.h>
#include <SPI.h>
#include <pico/stdlib.h>
#include <pico/unique_id.h>
#include <string.h>
#include <tusb.h>

#include "bt.h"
#include "commands.h"
#include "common/tusb_types.h"
#include "config.h"
#include "controllers.h"
#include "device/dcd.h"
#include "device/usbd_pvt.h"
#include "hardware/structs/usb.h"
#include "hardware/watchdog.h"
#include "host/usbh_classdriver.h"
#include "pico/bootrom.h"
#include "pico/cyw43_arch.h"
#include "pico/multicore.h"
#include "pins.h"
#include "reports/controller_reports.h"
#include "serial.h"
#include "shared_main.h"
#include "xinput_device.h"
#include "xinput_host.h"

CFG_TUSB_MEM_SECTION CFG_TUSB_MEM_ALIGN uint8_t buf[255];
CFG_TUSB_MEM_SECTION CFG_TUSB_MEM_ALIGN uint8_t buf2[255];
CFG_TUSB_MEM_SECTION CFG_TUSB_MEM_ALIGN STRING_DESCRIPTOR_PICO serialstring = {
    .bLength = (sizeof(uint8_t) + sizeof(uint8_t) + SERIAL_LEN),
    .bDescriptorType = USB_DESCRIPTOR_STRING,
    .UnicodeString = {}};

static uint32_t __uninitialized_ram(persistedConsoleType);
static uint32_t __uninitialized_ram(persistedConsoleTypeValid);
uint8_t xone_dev_addr = 0;
uint8_t x360_dev_addr = 0;
uint8_t ps4_dev_addr = 0;
bool connected = false;

typedef struct {
    uint8_t pin_dp;
    uint8_t pio_tx_num;
    uint8_t sm_tx;
    uint8_t tx_ch;
    uint8_t pio_rx_num;
    uint8_t sm_rx;
    uint8_t sm_eop;
    void *alarm_pool;
    int8_t debug_pin_rx;
    int8_t debug_pin_eop;
} pio_usb_configuration_t;

bool ready_for_next_packet() {
    return tud_xinput_n_ready(0) && tud_ready_for_packet();
}

bool usb_connected() {
    return connected;
}

void send_report_to_pc(const void *report, uint8_t len) {
    tud_xinput_n_report(0, 0, report, len);
}
void loop() {
    tick();
    tud_task();
#if USB_HOST_STACK
    // If a plugged in xbox one controller is trying to send us data, and we are authenticating, receive it
    if (xbox_one_state != Ready && xone_dev_addr && tuh_xinput_ready(xone_dev_addr, 0)) {
        tuh_xinput_receive_report(xone_dev_addr, 0);
    }
    tuh_task();
#endif
}
void setup() {
    generateSerialString(&serialstring);
#if USB_HOST_STACK
    pio_usb_configuration_t config = {
        USB_HOST_DP_PIN, 0, 0, 0, 1, 0, 1, NULL, -1, -1};
    tuh_configure(0, TUH_CFGID_RPI_PIO_USB_CONFIGURATION, &config);
#endif
    tusb_init();
    if (persistedConsoleTypeValid == 0x3A2F) {
        consoleType = persistedConsoleType;
    }
    printf("ConsoleType: %d\r\n", consoleType);
    init_main();
#if BLUETOOTH
    btstack_main();
#endif
}
void send_report_to_controller(uint8_t deviceType, uint8_t *report, uint8_t len) {
    uint8_t dev_addr = 0;
    if (deviceType == XBOXONE) {
        dev_addr = xone_dev_addr;
    }
    if (deviceType == WINDOWS_XBOX360) {
        dev_addr = x360_dev_addr;
    }
    if (deviceType == PS4) {
        dev_addr = ps4_dev_addr;
    }
    if (dev_addr && tuh_xinput_mounted(dev_addr, 0)) {
        tuh_xinput_send_report(dev_addr, 0, report, len);
    }
}
void tud_mount_cb(void) {
    device_reset();
    connected = true;
}

void tuh_xinput_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t controllerType, uint8_t subtype) {
    printf("Detected controller: %d\r\n", controllerType);
    if (controllerType == WINDOWS_XBOX360) {
        x360_dev_addr = dev_addr;
        uint16_t host_vid = 0;
        uint16_t host_pid = 0;
        tuh_vid_pid_get(dev_addr, &host_vid, &host_pid);
        xinput_controller_connected(host_vid, host_pid, subtype);
    } else if (controllerType == XBOXONE) {
        xone_dev_addr = dev_addr;
        xone_controller_connected();
    } else if (controllerType == UNKNOWN) {
        uint16_t host_vid = 0;
        uint16_t host_pid = 0;
        tuh_vid_pid_get(dev_addr, &host_vid, &host_pid);
        // Look for a PS4 controller, just attempt to retrieve auth data from it and if that succeeds then its a ps4 controller.
        if (!ps4_dev_addr) {
            ps4_dev_addr = dev_addr;
            uint8_t ret = transfer_with_usb_controller(PS4, (USB_SETUP_DEVICE_TO_HOST | USB_SETUP_RECIPIENT_INTERFACE | USB_SETUP_TYPE_CLASS), 1, 0x0303, 0, 48, buf);
            if (ret != 0x30) {
                ps4_dev_addr = 0;
                return;
            }
            ps4_controller_connected(host_vid, host_pid);
        }
    }
}

void tuh_xinput_umount_cb(uint8_t dev_addr, uint8_t instance) {
    xone_dev_addr = 0;
    x360_dev_addr = 0;
    ps4_dev_addr = 0;
    controller_disconnected();
}

uint8_t const *tud_descriptor_device_cb(void) {
    descriptorRequest(USB_DESCRIPTOR_DEVICE << 8, 0, buf);
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
        return (uint16_t *)&serialstring;
    }
    if (descriptorRequest(USB_DESCRIPTOR_STRING << 8 | index, 0, buf)) {
        return (uint16_t *)buf;
    }
    return NULL;
}
void tuh_xinput_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const *report, uint16_t len) {
    receive_report_from_controller(report, len);
}


uint8_t transfer_with_usb_controller(const uint8_t device, const uint8_t requestType, const uint8_t request, const uint16_t wValue, const uint16_t wIndex, const uint16_t wLength, uint8_t *buffer) {
    tusb_control_request_t setup = {
        bmRequestType : requestType,
        bRequest : request,
        wValue : wValue,
        wIndex : wIndex,
        wLength : wLength
    };
    tuh_xfer_t xfer = {};
    if (device == WINDOWS_XBOX360) {
        xfer.daddr = x360_dev_addr;
    } else if (device == PS4) {
        xfer.daddr = ps4_dev_addr;
    } else if (device == XBOXONE) {
        xfer.daddr = xone_dev_addr;
    }
    xfer.ep_addr = 0;
    xfer.setup = &setup;
    xfer.buffer = buffer;
    xfer.complete_cb = NULL;
    xfer.user_data = 0;
    tuh_control_xfer(&xfer);
    if (xfer.result != XFER_RESULT_SUCCESS) {
        return false;
    }
    return xfer.actual_len;
}
tusb_control_request_t lastreq;
bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request) {
    if (request->bmRequestType_bit.type == TUSB_REQ_TYPE_STANDARD && request->bRequest == TUSB_REQ_GET_DESCRIPTOR) {
        //------------- STD Request -------------//
        if (stage == CONTROL_STAGE_SETUP) {
            uint8_t const desc_type = tu_u16_high(request->wValue);
            // uint8_t const desc_index = tu_u16_low (request->wValue);

            if (request->bRequest == TUSB_REQ_GET_DESCRIPTOR && desc_type == HID_DESCRIPTOR_HID) {
                uint16_t len = descriptorRequest(HID_DESCRIPTOR_HID << 8, 0, buf);
                TU_VERIFY(tud_control_xfer(rhport, request, buf, len));
                return true;
            } else if (request->bRequest == TUSB_REQ_GET_DESCRIPTOR && desc_type == HID_DESCRIPTOR_REPORT) {
                uint16_t len = descriptorRequest(HID_DESCRIPTOR_REPORT << 8, 0, buf);
                TU_VERIFY(tud_control_xfer(rhport, request, buf, len));
                return true;
            } else {
                return false;  // stall unsupported request
            }
        }
    }
    if (controlRequestValid(request->bmRequestType, request->bRequest, request->wValue, request->wIndex, request->wLength)) {
        if (request->bmRequestType_bit.direction == TUSB_DIR_IN) {
            if (stage == CONTROL_STAGE_SETUP) {
                uint16_t len = controlRequest(request->bmRequestType, request->bRequest, request->wValue, request->wIndex, request->wLength, buf);
                tud_control_xfer(rhport, request, buf, len);
            }
        } else {
            if (stage == CONTROL_STAGE_SETUP) {
                tud_control_xfer(rhport, request, buf, request->wLength);
            }
            if (stage == CONTROL_STAGE_DATA || (stage == CONTROL_STAGE_SETUP && !request->wLength)) {
                controlRequest(request->bmRequestType, request->bRequest, request->wValue, request->wIndex, request->wLength, buf);
            }
        }
    }

    return true;
}

usbd_class_driver_t driver[] = {
    {
#if CFG_TUSB_DEBUG >= 2
        .name = "XInput_Device_HID",
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

usbh_class_driver_t driver_host[] = {
    {
#if CFG_TUSB_DEBUG >= 2
        .name = "XInput_Host_HID",
#endif
        .init = xinputh_init,
        .open = xinputh_open,
        .set_config = xinputh_set_config,
        .xfer_cb = xinputh_xfer_cb,
        .close = xinputh_close}};

usbh_class_driver_t const *usbh_app_driver_get_cb(uint8_t *driver_count) {
    *driver_count = 1;
    return driver_host;
}
void reboot(void) {
    watchdog_enable(1, false);
    for (;;) {
    }
}
void bootloader(void) {
    reset_usb_boot(0, 0);
}
void reset_usb(void) {
    persistedConsoleType = consoleType;
    persistedConsoleTypeValid = 0x3A2F;
    reboot();
}