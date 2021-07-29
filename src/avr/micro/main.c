#include <LUFA/Drivers/USB/USB.h>

#include "descriptors.h"
#include "lib_main.h"
#include "usb.h"
uint8_t controller[DEVICE_EPSIZE_IN];
int main(void) {
    init();
    USB_Init();
    sei();
    while (true) {
        uint8_t len = tick(controller);
        Endpoint_SelectEndpoint(DEVICE_EPADDR_IN);
        if (Endpoint_IsINReady()) {
            Endpoint_Write_Stream_LE(controller, len, NULL);
            Endpoint_ClearIN();
        }
        Endpoint_SelectEndpoint(DEVICE_EPADDR_OUT);
        if (Endpoint_IsOUTReceived()) {
            /* Check to see if the packet contains data */
            if (Endpoint_IsReadWriteAllowed()) {
                len = Endpoint_BytesInEndpoint();
                Endpoint_Read_Stream_LE(controller, len, NULL);
                packetReceived(controller, len);
                
            }
            Endpoint_ClearOUT();
        }
        USB_USBTask();
    }
}
// TODO: how big does this need to be?
uint8_t buf[255];
void EVENT_USB_Device_ControlRequest(void) {
    bool valid = false;
    uint8_t* data;
    requestType_t type = {bmRequestType : USB_ControlRequest.bmRequestType};
    if (type.bmRequestType_bit.direction == USB_DIR_DEVICE_TO_HOST) {
        uint16_t len = controlRequest(type, USB_ControlRequest.bRequest, USB_ControlRequest.wValue, USB_ControlRequest.wIndex, USB_ControlRequest.wLength, &data, &valid);
        if (valid) {
            Endpoint_ClearSETUP();
            Endpoint_Write_Control_Stream_LE(buf, len);
            Endpoint_ClearStatusStage();
        }
    } else if (type.bmRequestType_bit.type == USB_REQ_TYPE_VENDOR && type.bmRequestType_bit.recipient == USB_REQ_RCPT_INTERFACE) {
        data = buf;
        Endpoint_ClearSETUP();
        Endpoint_Read_Control_Stream_LE(buf, USB_ControlRequest.wLength);
        controlRequest(type, USB_ControlRequest.bRequest, USB_ControlRequest.wValue, USB_ControlRequest.wIndex, USB_ControlRequest.wLength, &data, &valid);
        Endpoint_ClearStatusStage();
    }
}
void EVENT_USB_Device_ConfigurationChanged(void) {
    uint8_t type = EP_TYPE_INTERRUPT;
    if (consoleType == MIDI) {
        type = EP_TYPE_BULK;
    }
    Endpoint_ConfigureEndpoint(DEVICE_EPADDR_IN, type, DEVICE_EPSIZE_IN, 1);
    Endpoint_ConfigureEndpoint(DEVICE_EPADDR_OUT, type, DEVICE_EPSIZE_OUT, 2);
}

uint16_t CALLBACK_USB_GetDescriptor(const uint16_t wValue,
                                    const uint16_t wIndex,
                                    const void** const descriptorAddress) {
    return descriptorRequest(wValue, wIndex, descriptorAddress);
}