#include <LUFA/Drivers/USB/USB.h>

#include "avr.h"
#include "descriptors.h"
#include "lib_main.h"
#include "timer.h"
#include "usb.h"
uint8_t controller[DEVICE_EPSIZE_IN];
int main(void) {
    init();
    setupMicrosTimer();
    // Since the micros timer doesn't rely on the crystal, we can compare it to our own timer, and if it is different then we know someone has programmed the wrong firmware
    long current = millis();
    _delay_ms(2);
    current = millis() - current;
    PLLCSR = 0;
    if ((current != 2 && F_CPU == 16000000) || F_CPU == 8000000) {
        PLLCSR = ((1 << PINDIV) | (1 << PLLE));
        realFreq = 8000000;
    } else if ((current != 2 && F_CPU == 8000000) || F_CPU == 16000000) {
        PLLCSR = (0 | (1 << PLLE));
        realFreq = 16000000;
    }
    // Now that we have calculated the real frequency, set up the timer again with the real speed
    setupMicrosTimer();
    USB_Init();
    sei();
    while (true) {
        // uint8_t len = tick(controller);
        // Endpoint_SelectEndpoint(DEVICE_EPADDR_IN);
        // if (Endpoint_IsINReady()) {
        //     Endpoint_Write_Stream_LE(controller, len, NULL);
        //     Endpoint_ClearIN();
        // }
        // Endpoint_SelectEndpoint(DEVICE_EPADDR_OUT);
        // if (Endpoint_IsOUTReceived()) {
        //     /* Check to see if the packet contains data */
        //     if (Endpoint_IsReadWriteAllowed()) {
        //         len = Endpoint_BytesInEndpoint();
        //         Endpoint_Read_Stream_LE(controller, len, NULL);
        //         packetReceived(controller, len);
        //     }
        //     Endpoint_ClearOUT();
        // }
        USB_USBTask();
    }
}
// TODO: how big does this need to be?
uint8_t buf[255];
void EVENT_USB_Device_ControlRequest(void) {
    bool valid = false;
    requestType_t type = {bmRequestType : USB_ControlRequest.bmRequestType};
    if (type.bmRequestType_bit.direction == USB_DIR_DEVICE_TO_HOST) {
        uint16_t len = controlRequest(type, USB_ControlRequest.bRequest, USB_ControlRequest.wValue, USB_ControlRequest.wIndex, USB_ControlRequest.wLength, &buf, &valid);
        if (valid) {
            Endpoint_ClearSETUP();
            Endpoint_Write_Control_Stream_LE(buf, len);
            Endpoint_ClearStatusStage();
        }
    } else if (type.bmRequestType_bit.type == USB_REQ_TYPE_VENDOR && type.bmRequestType_bit.recipient == USB_REQ_RCPT_INTERFACE) {
        Endpoint_ClearSETUP();
        Endpoint_Read_Control_Stream_LE(buf, USB_ControlRequest.wLength);
        controlRequest(type, USB_ControlRequest.bRequest, USB_ControlRequest.wValue, USB_ControlRequest.wIndex, USB_ControlRequest.wLength, &buf, &valid);
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
    *descriptorAddress = buf;
    return descriptorRequest(wValue, wIndex, buf);
}