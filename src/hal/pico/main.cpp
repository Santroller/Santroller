#include "Arduino.h"
#include "devices/bh_drum.hpp"
#include "devices/midi.hpp"
#include "hal/pico/interfaces/i2c.hpp"
#include "host/usbh.h"
#include "host/usbh_pvt.h"
#include "interfaces/i2c.hpp"
#include "xinput_host.h"
#include "parsers/xboxone.hpp"
typedef enum {
    PIO_USB_PINOUT_DPDM = 0,  // DM = DP+1
    PIO_USB_PINOUT_DMDP,      // DM = DP-1
} PIO_USB_PINOUT;
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
    PIO_USB_PINOUT pinout;
} pio_usb_configuration_t;
XboxOneParser parser;
san_base_t data;
uint8_t buf[256];
void loop() {
    tuh_task();
    int len = parser.build(&data, buf);
    if (len) {
        tuh_xinput_send_report(0, 0, buf, len);
    }
}
void setup() {
    pio_usb_configuration_t config = {
        pin_dp : 2,
        pio_tx_num : 0,
        sm_tx : 0,
        tx_ch : 0,
        pio_rx_num : 0,
        sm_rx : 1,
        sm_eop : 2,
        alarm_pool : NULL,
        debug_pin_rx : -1,
        debug_pin_eop : -1,
        skip_alarm_pool : false,
        pinout : PIO_USB_PINOUT_DPDM
    };
    tuh_configure(0, TUH_CFGID_RPI_PIO_USB_CONFIGURATION, &config);
    tuh_init(TUH_OPT_RHPORT);
}

bool tuh_xinput_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t console_type, uint8_t sub_type) {
    printf("Detected controller: %d (%d) on %d, %d\r\n", console_type, sub_type, dev_addr, instance);
    return true;
}

void tuh_xinput_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const *report, uint16_t len) {
    memcpy(buf, report, len);
    parser.parse(buf, len, &data);
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