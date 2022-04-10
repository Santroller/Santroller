/**
 * You should have a LUFAConfig.h for this to work.
 */

#include "LUFAConfig.h"
/**
 * Include LUFA.h after LUFAConfig.h
 */
#include <Wire.h>
#include <SPI.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/power.h>
#include <avr/interrupt.h>
#include <string.h>
#include "config.h"

#include "descriptors.h"
#include "bootloader.h"

#include <LUFA.h>
#include <LUFA/LUFA/Drivers/Board/LEDs.h>
#include <LUFA/LUFA/Drivers/USB/USB.h>
#include <LUFA/LUFA/Drivers/USB/Class/CDCClass.h>
#include <LUFA/LUFA/Platform/Platform.h>

void SetupHardware(void);

volatile bool waiting = true;
void setup()
{
	GlobalInterruptEnable(); // enable global interrupts
	SetupHardware(); // ask LUFA to setup the hardware
}

void loop()
{
	USB_USBTask();
}

void SetupHardware(void)
{
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
    bool valid = false;
    if ((USB_ControlRequest.bmRequestType & CONTROL_REQTYPE_DIRECTION) == (REQDIR_DEVICETOHOST)) {
        // uint16_t len = controlRequest(type, USB_ControlRequest.bRequest, USB_ControlRequest.wValue, USB_ControlRequest.wIndex, USB_ControlRequest.wLength, &buf, &valid);
        // if (valid) {
        //     Endpoint_ClearSETUP();
        //     Endpoint_Write_Control_Stream_LE(buf, len);
        //     Endpoint_ClearStatusStage();
        // }
    } else if (USB_ControlRequest.bmRequestType == (REQDIR_DEVICETOHOST | REQTYPE_VENDOR | REQREC_INTERFACE)) {
        // Endpoint_ClearSETUP();
        // Endpoint_Read_Control_Stream_LE(buf, USB_ControlRequest.wLength);
        // controlRequest(type, USB_ControlRequest.bRequest, USB_ControlRequest.wValue, USB_ControlRequest.wIndex, USB_ControlRequest.wLength, &buf, &valid);
        // Endpoint_ClearStatusStage();
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
ISR(WDT_vect) {
    wdt_disable();
    waiting = false;
}
void reset_usb(void) {
    // TODO: is this working?
    USB_Disable();
    USB_Init();
}