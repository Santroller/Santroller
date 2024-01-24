#include <Arduino.h>
#include <SPI.h>
#include <pico/stdlib.h>
#include <pico/unique_id.h>
#include <string.h>
#include <tusb.h>

#include "bt.h"
#include "btstack_run_loop.h"
#include "commands.h"
#include "common/tusb_types.h"
#include "config.h"
#include "controllers.h"
#include "device/dcd.h"
#include "device/usbd_pvt.h"
#include "hardware/structs/usb.h"
#include "hardware/watchdog.h"
#include "hidescriptorparser.h"
#include "host/usbh_classdriver.h"
#include "pico/bootrom.h"
#include "pico/cyw43_arch.h"
#include "pico/multicore.h"
#include "pin_funcs.h"
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
#define PERSISTED_CONSOLE_TYPE_VALID 0x3A2F
static uint32_t __uninitialized_ram(persistedConsoleType);
static uint32_t __uninitialized_ram(windows_in_hid);
static uint32_t __uninitialized_ram(persistedConsoleTypeValid);
uint8_t xone_dev_addr = 0;
uint8_t x360_dev_addr = 0;
uint8_t ps4_dev_addr = 0;
bool connected = false;
#if USB_HOST_STACK
uint8_t total_usb_host_devices = 0;
typedef struct {
    USB_Device_Type_t type;
    USB_LastReport_Data_t report;
    uint8_t report_length;
    uint8_t tap_bar_type;
} Usb_Host_Device_t;

Usb_Host_Device_t usb_host_devices[CFG_TUH_DEVICE_MAX];
#endif
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
    bool skip_alarm_pool;
} pio_usb_configuration_t;
uint8_t prev_bt_report[32];

bool ready_for_next_packet() {
    return tud_xinput_n_ready(0) && tud_ready_for_packet();
}

bool usb_configured() {
    return connected;
}

void send_report_to_pc(const void *report, uint8_t len) {
    tud_xusb_n_report(0, report, len);
}
bool foundXB = false;
bool authReady = false;
bool authDone = false;

static void tick_usb() {
#if USB_HOST_STACK
    if (consoleType == XBOX360 && !foundXB && XINPUT_AUTH) {
        tuh_task();
        return;
    }
    if (consoleType == XBOXONE && !foundXB) {
        tuh_task();
        return;
    }
#endif
#if !BLUETOOTH
    tud_task();
#endif
#if USB_HOST_STACK
    tuh_task();
#endif
#ifdef BLUETOOTH_RX
    // if connected to the transmitter, then run the bt based tick, otherwise run the usb based tick.
    if (!check_bluetooth_ready()) {
        tick();
    }
#else
    tick();
#endif
#if BLUETOOTH
    if (!authDone) {
        if (consoleType != XBOXONE && (consoleType != XBOX360 || !XINPUT_AUTH)) {
            authReady = millis() > 1000;
        }
        if (authReady) {
            btstack_main();
            authDone = true;
        }
    }
#endif
}
#ifdef BLUETOOTH_RX
void tick_bluetooth(const void *buf, uint8_t len) {
    tick_bluetooth_inputs(buf);
}
#endif
#if BLUETOOTH
void loop1() {
    tud_task();
}
#endif
void loop() {
    tick_usb();
}
void setup() {
    if (persistedConsoleTypeValid == PERSISTED_CONSOLE_TYPE_VALID) {
        consoleType = persistedConsoleType;
    } else {
        windows_in_hid = false;
        consoleType = UNIVERSAL;
    }
    generateSerialString(&serialstring, consoleType);
    printf("ConsoleType: %d\r\n", consoleType);
    init_main();
    tud_init(TUD_OPT_RHPORT);
#if USB_HOST_STACK
    set_sys_clock_khz(120000, true);
    pio_usb_configuration_t config = {
        USB_HOST_DP_PIN, 0, 0, 0, 1, 0, 1, NULL, -1, -1, .skip_alarm_pool = false};
    tuh_configure(0, TUH_CFGID_RPI_PIO_USB_CONFIGURATION, &config);
    tuh_init(TUH_OPT_RHPORT);
#endif
}
void authentication_successful() {
    printf("Auth done\r\n");
    authReady = true;
}

uint8_t get_device_address_for(uint8_t deviceType) {
    if (deviceType == XBOXONE) {
        return xone_dev_addr;
    }
    if (deviceType == XBOX360) {
        return x360_dev_addr;
    }
    if (deviceType == PS4) {
        return ps4_dev_addr;
    }
    return 0;
}

void send_report_to_controller(uint8_t dev_addr, uint8_t *report, uint8_t len) {
    if (dev_addr && tuh_xinput_mounted(dev_addr, 0)) {
        tuh_xinput_send_report(dev_addr, 0, report, len);
    }
}
void tud_mount_cb(void) {
    device_reset();
    connected = true;
}
#if USB_HOST_STACK
uint8_t get_usb_host_device_count() {
    return total_usb_host_devices;
}
USB_Device_Type_t get_usb_host_device_type(uint8_t id) {
    return usb_host_devices[id].type;
}
uint8_t get_usb_host_device_tap_bar_type(uint8_t id) {
    return usb_host_devices[id].tap_bar_type;
}

void get_usb_host_device_data(uint8_t id, uint8_t *buf) {
    memcpy(buf, &usb_host_devices[id].report, usb_host_devices[id].report_length);
}

uint8_t read_usb_host_devices(uint8_t *buf) {
    for (int i = 0; i < total_usb_host_devices; i++) {
        USB_Device_Type_t *type = &usb_host_devices[i].type;
        buf[(i * 2)] = type->console_type;
        buf[(i * 2) + 1] = type->sub_type;
    }
    return total_usb_host_devices * 2;
}

void tuh_xinput_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t controllerType, uint8_t subtype) {
    printf("Detected controller: %d (%d) on %d\r\n", controllerType, subtype, dev_addr);
    uint16_t host_vid = 0;
    uint16_t host_pid = 0;
    tuh_vid_pid_get(dev_addr, &host_vid, &host_pid);
    USB_Device_Type_t type = get_usb_device_type_for(host_vid, host_pid);
    type.dev_addr = dev_addr;
    if (controllerType == XBOX360) {
        if (subtype) {
            type.console_type = controllerType;
            type.sub_type = subtype;

            x360_dev_addr = dev_addr;
            xinput_controller_connected(host_vid, host_pid, subtype);
            usb_host_devices[total_usb_host_devices].type = type;
            total_usb_host_devices++;
            if (consoleType == XBOX360) {
                foundXB = true;
            }
        }
    } else if (controllerType == XBOXONE) {
        xone_dev_addr = dev_addr;
        type.console_type = XBOXONE;
        xone_controller_connected(dev_addr);
        usb_host_devices[total_usb_host_devices].type = type;
        total_usb_host_devices++;
        if (consoleType == XBOXONE) {
            foundXB = true;
        }

    } else if (controllerType == UNKNOWN) {
        if (type.console_type) {
            usb_host_devices[total_usb_host_devices].type = type;
            total_usb_host_devices++;
            if (type.console_type == PS3) {
                printf("Found PS3 controller\r\n");
                ps3_controller_connected(dev_addr, host_vid, host_pid);
            }
        }
    } else if (controllerType == PS4) {
        type.console_type = PS4;
        usb_host_devices[total_usb_host_devices].type = type;
        total_usb_host_devices++;
        if (!ps4_dev_addr) {
            ps4_dev_addr = dev_addr;

            printf("Found PS4 controller\r\n");
            ps4_controller_connected(dev_addr, host_vid, host_pid);
        }
    }
    printf("Total devices: %d\r\n", total_usb_host_devices);

    host_controller_connected();
}

void tuh_xinput_umount_cb(uint8_t dev_addr, uint8_t instance) {
    printf("Unplugged %d\r\n", dev_addr);
    if (xone_dev_addr == dev_addr) {
        xone_dev_addr = 0;
    }
    if (x360_dev_addr == dev_addr) {
        x360_dev_addr = 0;
    }
    if (ps4_dev_addr == dev_addr) {
        ps4_dev_addr = 0;
        ps4_controller_disconnected();
    }
    // Probably should actulaly work out what was unplugged and all that
    total_usb_host_devices = 0;
}
bool wasXb1Input = false;
void tuh_xinput_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const *report, uint16_t len) {
    if (dev_addr == xone_dev_addr) {
        receive_report_from_controller(report, len);
    }
    for (int i = 0; i < total_usb_host_devices; i++) {
        if (usb_host_devices[i].type.dev_addr == dev_addr) {
            if (usb_host_devices[i].type.console_type == XBOXONE) {
                GipHeader_t *header = (GipHeader_t *)report;
                if (header->command == GIP_VIRTUAL_KEYCODE) {
                    GipKeystroke_t *keystroke = (GipKeystroke_t *)report;
                    if (wasXb1Input) {
                        XboxOneInputHeader_Data_t *gamepad = (XboxOneInputHeader_Data_t *)(&(usb_host_devices[i].report));
                        gamepad->guide = keystroke->pressed;
                    }
                    return;
                }
                if (header->command != GHL_HID_REPORT && header->command != GIP_INPUT_REPORT) {
                    return;
                }
                wasXb1Input = header->command == GIP_INPUT_REPORT;
            }
            memcpy(&usb_host_devices[i].report, report, len);
            usb_host_devices[i].report_length = len;
            return;
        }
    }
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
#endif

uint8_t const *tud_descriptor_device_cb(void) {
    descriptorRequest(USB_DESCRIPTOR_DEVICE << 8, 0, buf);
    return buf;
}
uint8_t const *tud_hid_custom_descriptor_report_cb(uint8_t instance) {
    descriptorRequest(HID_DESCRIPTOR_REPORT << 8, instance, buf);
    return buf;
}
uint8_t const *tud_descriptor_configuration_cb(uint8_t index) {
    (void)index;  // for multiple configurations
    descriptorRequest(USB_DESCRIPTOR_CONFIGURATION << 8, index, buf);
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

uint8_t transfer_with_usb_controller(const uint8_t dev_addr, const uint8_t requestType, const uint8_t request, const uint16_t wValue, const uint16_t wIndex, const uint16_t wLength, uint8_t *buffer) {
    if (!dev_addr) {
        // Device is not connected yet!
        return 0;
    }
    tusb_control_request_t setup = {
        bmRequestType : requestType,
        bRequest : request,
        wValue : wValue,
        wIndex : wIndex,
        wLength : wLength
    };
    tuh_xfer_t xfer = {};
    xfer.daddr = dev_addr;
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
                uint16_t len = descriptorRequest(HID_DESCRIPTOR_HID << 8, request->wIndex, buf);
                TU_VERIFY(tud_control_xfer(rhport, request, buf, len));
                return true;
            } else if (request->bRequest == TUSB_REQ_GET_DESCRIPTOR && desc_type == HID_DESCRIPTOR_REPORT) {
                uint16_t len = descriptorRequest(HID_DESCRIPTOR_REPORT << 8, request->wIndex, buf);
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
    persistedConsoleTypeValid = PERSISTED_CONSOLE_TYPE_VALID;
    reboot();
}