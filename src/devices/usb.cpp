#include "devices/usb.hpp"
#include "tusb.h"
#include "pio_usb.h"
#include "host/usbh_pvt.h"

// void USBDevice::tick(san_base_t* data) {
//     mParser.parse(mReportData, sizeof(mReportData), data);
//     if (data->midi.midiPacket[0]) {
//         midiInterface.parsePacket(data->midi.midiPacket, sizeof(data->midi.midiPacket));
//     }
// }

USBDevice::USBDevice(proto_UsbHostDevice device, uint16_t id) : Device(id), m_device(device)
{
    pio_usb_configuration_t host_config = {
        pin_dp : device.firstPin + device.dmFirst,
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
        pinout : device.dmFirst ? PIO_USB_PINOUT_DMDP : PIO_USB_PINOUT_DPDM
    };
    tuh_configure(0, TUH_CFGID_RPI_PIO_USB_CONFIGURATION, &host_config);
    tuh_init(TUH_OPT_RHPORT);
}

void USBDevice::update(bool full_poll)
{
}