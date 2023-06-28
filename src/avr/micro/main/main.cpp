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
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/power.h>
#include <avr/wdt.h>
#include <string.h>

#include "commands.h"
#include "config.h"
#include "descriptors.h"
#include "hid.h"
#include "shared_main.h"

volatile uint16_t test __attribute__((section(".noinit")));
volatile uint16_t test2 __attribute__((section(".noinit")));
void SetupHardware(void);

bool connected = false;
void EVENT_USB_Device_Connect(void) {
    connected = true;
}

void EVENT_USB_Device_Disconnect(void) {
    connected = false;
}

bool usb_connected() {
    return connected;
}

void setup() {
    init_main();
    if (test2 == 0x3A2F) {
        consoleType = test;
    }
    GlobalInterruptEnable();  // enable global interrupts
    SetupHardware();          // ask LUFA to setup the hardware
}

uint8_t buf[255];
void loop() {
    tick();
    Endpoint_SelectEndpoint(DEVICE_EPADDR_OUT);
    if (Endpoint_IsOUTReceived()) {
        if (Endpoint_IsReadWriteAllowed()) {
            uint8_t size = Endpoint_BytesInEndpoint();
            Endpoint_Read_Stream_LE(buf, size, NULL);
            hid_set_report(buf, size, INTERRUPT_ID, INTERRUPT_ID);
        }
        Endpoint_ClearOUT();
    }
}
bool ready_for_next_packet() {
    Endpoint_SelectEndpoint(DEVICE_EPADDR_IN);
    return Endpoint_IsINReady();
}
void send_report_to_pc(const void* report, uint8_t len) {
    Endpoint_SelectEndpoint(DEVICE_EPADDR_IN);
    Endpoint_Write_Stream_LE(report, len, NULL);
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

void EVENT_USB_Device_ControlRequest(void) {
    if (controlRequestValid(USB_ControlRequest.bmRequestType, USB_ControlRequest.bRequest, USB_ControlRequest.wValue, USB_ControlRequest.wIndex, USB_ControlRequest.wLength)) {
        if ((USB_ControlRequest.bmRequestType & CONTROL_REQTYPE_DIRECTION) == (REQDIR_DEVICETOHOST)) {
            uint16_t len = controlRequest(USB_ControlRequest.bmRequestType, USB_ControlRequest.bRequest, USB_ControlRequest.wValue, USB_ControlRequest.wIndex, USB_ControlRequest.wLength, buf);
            Endpoint_ClearSETUP();
            Endpoint_Write_Control_Stream_LE(buf, len);
            Endpoint_ClearStatusStage();
        } else {
            Endpoint_ClearSETUP();
            Endpoint_Read_Control_Stream_LE(buf, USB_ControlRequest.wLength);
            controlRequest(USB_ControlRequest.bmRequestType, USB_ControlRequest.bRequest, USB_ControlRequest.wValue, USB_ControlRequest.wIndex, USB_ControlRequest.wLength, buf);
            Endpoint_ClearStatusStage();
        }
    }
}
void EVENT_USB_Device_ConfigurationChanged(void) {
    uint8_t type = EP_TYPE_INTERRUPT;
    uint8_t epsize = 0x20;
    uint8_t epsizeOut = 0x08;
    if (consoleType == WINDOWS || consoleType == XBOX360) {
        epsize = 0x18;
    }
    if (consoleType == XBOXONE) {
        epsize = 0x40;
        epsizeOut = 0x40;
    }
    if (consoleType == MIDI) {
        type = EP_TYPE_BULK;
    }
    Endpoint_ConfigureEndpoint(DEVICE_EPADDR_IN, type, epsize, 2);
    Endpoint_ConfigureEndpoint(DEVICE_EPADDR_OUT, type, epsizeOut, 2);
}

uint16_t CALLBACK_USB_GetDescriptor(const uint16_t wValue,
                                    const uint16_t wIndex,
                                    const void** const descriptorAddress) {
    *descriptorAddress = buf;
    return descriptorRequest(wValue, wIndex, buf);
}
void reset_usb(void) {
    test = consoleType;
    test2 = 0x3A2F;
    reboot();
}