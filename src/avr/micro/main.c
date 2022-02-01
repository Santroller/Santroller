#include <LUFA/Drivers/USB/USB.h>

#include "avr.h"
#include "avr/wdt.h"
#include "descriptors.h"
#include "lib_main.h"
#include "timer.h"
#include "usb.h"
volatile bool waiting = true;
uint8_t controller[0x20];
int main(void) {
    init();
    cli();
    wdt_reset();
    // Enable the watchdog timer, as it runs from an internal clock so it will not be affected by the crystal
    MCUSR &= ~_BV(WDRF);
    /* Start the WDT Config change sequence. */
    WDTCSR |= _BV(WDCE) | _BV(WDE);
    /* Configure the prescaler and the WDT for interrupt mode only*/
    WDTCSR = _BV(WDIE) | WDTO_15MS;
    sei();
    setupMicrosTimer();
    // work out how many milliseconds it takes for the WDT to trigger
    long timeSinceWDT = millis();
    while (waiting) {
    }
    timeSinceWDT = millis() - timeSinceWDT;
    // And now compare to what we expect
    if (F_CPU == 8000000 && timeSinceWDT > 20) {
        // if the user is running at 16mhz, then it will run twice as fast, thus it will take longer than 15ms
        realFreq = 16000000;
        scaleDiv2 = true;
    } else if (F_CPU == 16000000 && timeSinceWDT < 8) {
        // if the user is running at 8mhz, then it will run at half speed, thus it will take less than 15ms
        realFreq = 8000000;
        scaleMul2 = true;
    }
    PLLCSR = 0;
    if (realFreq == 8000000) {
        PLLCSR = (0 | (1 << PLLE));
    } else if (realFreq == 16000000) {
        PLLCSR = ((1 << PINDIV) | (1 << PLLE));
    }

    // Now that we have calculated the real frequency, set up the timer again with the real speed
    setupMicrosTimer();
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
    uint8_t epsize = 0x20;
    if (consoleType == MIDI) {
        type = EP_TYPE_BULK;
    }
    if (consoleType == XBOX360) {
        epsize = 0x18;
    }
    Endpoint_ConfigureEndpoint(DEVICE_EPADDR_IN, type, epsize, 1);
    Endpoint_ConfigureEndpoint(DEVICE_EPADDR_OUT, type, 0x08, 2);
}

uint16_t CALLBACK_USB_GetDescriptor(const uint16_t wValue,
                                    const uint16_t wIndex,
                                    const void** const descriptorAddress) {
    *descriptorAddress = buf;
    return descriptorRequest(wValue, wIndex, buf);
}
ISR(WDT_vect) {
    wdt_disable();
    waiting = false;
}
void reset_usb(void) {
    // TODO: is this working?
    USB_Disable();
    USB_Init();
}