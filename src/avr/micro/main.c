#include <LUFA/Drivers/USB/USB.h>

#include "descriptors.h"
#include "lib_main.h"
#include "usb.h"
int main(void) {
    init();
    USB_Init();
    sei();
    while (true) {
        tick();
        USB_USBTask();
    }
}
// TODO: how big does this need to be?
uint8_t buf[255];
void EVENT_USB_Device_ControlRequest(void) {
    uint8_t* data;
    requestType_t type = {bmRequestType : USB_ControlRequest.bmRequestType};
    if (type.bmRequestType_bit.direction == USB_DIR_DEVICE_TO_HOST) {
        if (controlRequest(type, USB_ControlRequest.bRequest, USB_ControlRequest.wValue, USB_ControlRequest.wIndex, USB_ControlRequest.wLength, &data)) {
            Endpoint_Read_Control_Stream_LE(buf, USB_ControlRequest.wLength);
        }
    } else {
        data = buf;
        Endpoint_Read_Control_Stream_LE(buf, USB_ControlRequest.wLength);
        controlRequest(type, USB_ControlRequest.bRequest, USB_ControlRequest.wValue, USB_ControlRequest.wIndex, USB_ControlRequest.wLength, &data);
    }
}
void EVENT_USB_Device_ConfigurationChanged(void) {
    // TODO: change to EP_TYPE_BULK for midi
    Endpoint_ConfigureEndpoint(DEVICE_EPADDR_IN, EP_TYPE_INTERRUPT, HID_EPSIZE_IN,
                               1);
    Endpoint_ConfigureEndpoint(DEVICE_EPADDR_OUT, EP_TYPE_INTERRUPT, HID_EPSIZE_OUT,
                               1);
}

uint16_t CALLBACK_USB_GetDescriptor(const uint16_t wValue,
                                    const uint16_t wIndex,
                                    const void** const descriptorAddress) {
    return descriptorRequest(wValue, wIndex, descriptorAddress);
}