/**
 * You should have a LUFAConfig.h for this to work.
 */
#include "LUFAConfig.h"

/**
 * Include LUFA.h after LUFAConfig.h
 */
#include <LUFA.h>
#include <LUFA/LUFA/Drivers/Board/Joystick.h>
#include <LUFA/LUFA/Drivers/Board/LEDs.h>
#include <LUFA/LUFA/Drivers/USB/USB.h>
#include <LUFA/LUFA/Platform/Platform.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/power.h>
#include <avr/wdt.h>
#include <string.h>

void setup() {
    /* Disable watchdog if enabled by bootloader/fuses */
    MCUSR &= ~(1 << WDRF);
    wdt_disable();

    /* Disable clock division */
    clock_prescale_set(clock_div_1);
    USB_Init();

    GlobalInterruptEnable();  // enable global interrupts
}

void loop() {
    delayMicroseconds(100);
}
uint16_t CALLBACK_USB_GetDescriptor(const uint16_t wValue,
                                    const uint16_t wIndex,
                                    const void** const descriptorAddress) {
    return 0;
}