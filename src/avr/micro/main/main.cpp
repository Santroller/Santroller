/**
 * You should have a LUFAConfig.h for this to work.
 */

#include "LUFAConfig.h"
/**
 * Include LUFA.h after LUFAConfig.h
 */
#include <LUFA.h>
#include <LUFA/LUFA/Drivers/Board/LEDs.h>
#include <LUFA/LUFA/Drivers/USB/Class/CDCClass.h>
#include <LUFA/LUFA/Drivers/USB/USB.h>
#include <LUFA/LUFA/Platform/Platform.h>
#include <SPI.h>
#include <Wire.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/power.h>
#include <avr/wdt.h>
#include <string.h>

#include "commands.h"
#include "config.h"
#include "descriptors.h"
#include "shared_main.h"

void SetupHardware(void);

void setup() {
    init_main();
    GlobalInterruptEnable();  // enable global interrupts
    SetupHardware();          // ask LUFA to setup the hardware
}

REPORT_TYPE report;
void loop() {
    USB_USBTask();
    tick(&report);
    Endpoint_SelectEndpoint(DEVICE_EPADDR_IN);
    Endpoint_Write_Stream_LE(&report, sizeof(REPORT_TYPE), NULL);
    Endpoint_ClearIN();
}

void SetupHardware(void) {
    /* Disable watchdog if enabled by bootloader/fuses */
    MCUSR &= ~(1 << WDRF);
    wdt_disable();

    /* Disable clock division */
    clock_prescale_set(clock_div_1);

    /* Hardware Initialization */
    USB_Init();
}

uint8_t buf[255];
void EVENT_USB_Device_ControlRequest(void) {
    if (controlRequestValid(USB_ControlRequest.bmRequestType, USB_ControlRequest.bRequest, USB_ControlRequest.wValue, USB_ControlRequest.wIndex, USB_ControlRequest.wLength)) {
        if ((USB_ControlRequest.bmRequestType & CONTROL_REQTYPE_DIRECTION) == (REQDIR_DEVICETOHOST)) {
            uint16_t len = controlRequest(USB_ControlRequest.bmRequestType, USB_ControlRequest.bRequest, USB_ControlRequest.wValue, USB_ControlRequest.wIndex, USB_ControlRequest.wLength, &buf);
            Endpoint_ClearSETUP();
            Endpoint_Write_Control_Stream_LE(buf, len);
            Endpoint_ClearStatusStage();
        } else {
            Endpoint_ClearSETUP();
            Endpoint_Read_Control_Stream_LE(buf, USB_ControlRequest.wLength);
            controlRequest(USB_ControlRequest.bmRequestType, USB_ControlRequest.bRequest, USB_ControlRequest.wValue, USB_ControlRequest.wIndex, USB_ControlRequest.wLength, &buf);
            Endpoint_ClearStatusStage();
        }
    }
}
void EVENT_USB_Device_ConfigurationChanged(void) {
    uint8_t type = EP_TYPE_INTERRUPT;
    uint8_t epsize = 0x20;
    if (consoleType == MIDI) {
        type = EP_TYPE_BULK;
    }
    if (consoleType == XBOX360 || consoleType == PC_XINPUT) {
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
void reset_usb(void) {
    USB_Disable();
    USB_Init();
}