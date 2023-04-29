/**
 * You should have a LUFAConfig.h for this to work.
 */

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

#include "bootloader.h"
#include "descriptors_detect.h"
#include "wcid.h"

void SetupHardware(void);
char freqString[10];
volatile bool waiting = true;

const OS_COMPATIBLE_ID_DESCRIPTOR_SINGLE DevCompatIDs = {
    TotalLength : sizeof(OS_COMPATIBLE_ID_DESCRIPTOR_SINGLE),
    Version : 0x0100,
    Index : DESC_EXTENDED_COMPATIBLE_ID_DESCRIPTOR,
    TotalSections : 1,
    Reserved : {0},
    CompatID : {
        {
            FirstInterfaceNumber : INTERFACE_ID_Device,
            Reserved : 0x04,
            CompatibleID : "WINUSB",
            SubCompatibleID : {0},
            Reserved2 : {0}
        }}
};
void setup() {
    GlobalInterruptEnable();  // enable global interrupts
    wdt_reset();
    // Enable the watchdog timer, as it runs from an internal clock so it will not be affected by the crystal
    MCUSR &= ~_BV(WDRF);
    /* Start the WDT Config change sequence. */
    WDTCSR |= _BV(WDCE) | _BV(WDE);
    /* Configure the prescaler and the WDT for interrupt mode only*/
    WDTCSR = _BV(WDIE) | WDTO_15MS;
    sei();
    long timeSinceWDT = millis();
    while (waiting) {
    }
    timeSinceWDT = millis() - timeSinceWDT;
    realFreq = 16;
    // And now compare to what we expect
    if (F_CPU == 16000000 && timeSinceWDT < 10) {
        // if the user is running at 8mhz, then it will run at half speed, thus it will take less than 15ms
        realFreq = 8;
    }
    PLLCSR = 0;
    if (realFreq == 8) {
        PLLCSR = (0 | (1 << PLLE));
    } else if (realFreq == 16) {
        PLLCSR = ((1 << PINDIV) | (1 << PLLE));
    }
    itoa(realFreq, freqString, 10);
    freqString[strlen(freqString)] = '\n';
    SetupHardware();  // ask LUFA to setup the hardware
}

void loop() {
}

/** Configures the board hardware and chip peripherals for the demo's functionality. */
void SetupHardware(void) {
    /* Disable watchdog if enabled by bootloader/fuses */
    MCUSR &= ~(1 << WDRF);
    wdt_disable();

    /* Disable clock division */
    clock_prescale_set(clock_div_1);

    /* Hardware Initialization */
    USB_Init();
}

ISR(WDT_vect) {
    wdt_disable();
    waiting = false;
}

/** Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged(void) {
}

/** Event handler for the library USB Control Request reception event. */
void EVENT_USB_Device_ControlRequest(void) {
    if (USB_ControlRequest.bmRequestType == 0xC0 && USB_ControlRequest.bRequest == REQ_GET_OS_FEATURE_DESCRIPTOR && USB_ControlRequest.wIndex == DESC_EXTENDED_COMPATIBLE_ID_DESCRIPTOR) {
        Endpoint_ClearSETUP();
        Endpoint_Write_Control_Stream_LE(&DevCompatIDs, sizeof(DevCompatIDs));
        Endpoint_ClearStatusStage();
    }
}
