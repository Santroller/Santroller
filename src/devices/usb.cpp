#include "devices/usb.hpp"
#include "tusb.h"
#include "pio_usb.h"
#include "host/usbh_pvt.h"
#include "config.hpp"

USBDevice::USBDevice(proto_UsbHostDevice device, uint16_t id) : MidiDevice(id), m_device(device)
{
    if (device.firstPin == -1) {
        return;
    }
    pio_usb_configuration_t host_config = {
        pin_dp : (uint8_t)(device.firstPin + device.dmFirst),
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
    tuh_configure(TUH_OPT_RHPORT, TUH_CFGID_RPI_PIO_USB_CONFIGURATION, &host_config);
    tuh_init(TUH_OPT_RHPORT);
}

void USBDevice::update(bool full_poll)
{
}
uint16_t USBDevice::readMidiNote(uint8_t note)
{
    return midiInterface.midiVelocities[note] << 8;
}
uint16_t USBDevice::readMidiControlChange(uint8_t cc)
{
    switch (cc)
    {
    case MIDI_CONTROL_COMMAND_MOD_WHEEL:
        return midiInterface.midiModWheel << 8;
    case MIDI_CONTROL_COMMAND_SUSTAIN_PEDAL:
        return midiInterface.midiSustainPedal << 8;
    default:
        return 0;
    }
    return 0;
}
int16_t USBDevice::readMidiPitchBend()
{
    return midiInterface.midiPitchWheel;
}


void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t idx, uint8_t const *report, uint16_t len)
{
    // printf("report! %d\r\n", len);
    // if (report[0] == 0x81 && report[1] == 0x01)
    // {
    //     uint8_t buf[2] = {0x80 /* PROCON_REPORT_SEND_USB */, 0x02 /* PROCON_USB_HANDSHAKE */};
    //     tuh_hid_send_report(dev_addr, idx, 0, buf, 2);
    //     send_timeout = false;
    // }
    // else if (!send_timeout && report[0] == 0x81 && report[1] == 0x02)
    // {
    //     send_timeout = true;
    //     uint8_t buf[2] = {0x80 /* PROCON_REPORT_SEND_USB */, 0x03 /* PROCON_USB_ENABLE */};
    //     tuh_hid_send_report(dev_addr, idx, 0, buf, 2);
    // }
    // else if (report[0] == 0x81 && report[1] == 0x03)
    // {
    //     uint8_t buf[2] = {0x80 /* PROCON_REPORT_SEND_USB */, 0x02 /* PROCON_USB_HANDSHAKE */};
    //     tuh_hid_send_report(dev_addr, idx, 0, buf, 2);
    // }
    // else if (report[0] == 0x81 && report[1] == 0x02)
    // {
    //     uint8_t buf[2] = {0x80 /* PROCON_REPORT_SEND_USB */, 0x04 /* PROCON_USB_ENABLE */};
    //     tuh_hid_send_report(dev_addr, idx, 0, buf, 2);
    // }
    // if (report[0] == 0x30)
    // {
    //     for (int i = 0; i < len; i++)
    //     {
    //         printf("%02x, ", report[i]);
    //     }
    //     printf("\r\n");
    // }
    // tuh_hid_receive_report(dev_addr, idx);
}
void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t idx, uint8_t const *report_desc, uint16_t desc_len)
{

    printf("mount! %d\r\n", desc_len);
    tuh_hid_receive_report(dev_addr, idx);
}
bool USBDevice::using_pin(uint8_t pin)
{
    return pin == m_device.firstPin || pin == m_device.firstPin + 1;
}