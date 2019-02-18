#pragma once
#ifdef __cplusplus
extern "C" {
#endif
/* Includes: */
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/power.h>
#include <avr/wdt.h>
#include <string.h>
#include "../Controller.h"

#include "../lufa/Descriptors.h"

/* Type Defines: */
/** Type define for the joystick HID report structure, for creating and sending
 * HID reports to the host PC. This mirrors the layout described to the host in
 * the HID report descriptor, in Descriptors.c.
 */

extern USB_JoystickReport_Data_t gamepad_state;

/* Function Prototypes: */

void xbox_reset_pad_status(void);
void xbox_send_pad_state(void);
void xbox_reset_watchdog(void);
void xbox_init(bool watchdog);

void xbox_set_connect_callback(void (*callbackPtr)(void));
void xbox_set_disconnect_callback(void (*callbackPtr)(void));
#ifdef __cplusplus
}
#endif