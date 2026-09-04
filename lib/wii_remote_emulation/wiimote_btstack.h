#ifndef _WIIMOTE_BTSTACK_H_
#define _WIIMOTE_BTSTACK_H_

#include <stdint.h>

void wiimote_emulator_set_led(void (*led_on)(), void (*led_off)());

// Address of a previously-bonded console to actively reconnect to on startup, like a
// real Wiimote does. Must be set before/soon after wiimote_emulator() - it is consumed
// once the stack becomes discoverable/connectable.
void wiimote_emulator_set_reconnect_address(const uint8_t *mac);
// Called with the peer's BD address whenever a HID connection is established, so the
// caller can persist it (e.g. to reconnect after a reboot).
void wiimote_emulator_set_connection_callback(void (*callback)(const uint8_t *mac));

void wiimote_emulator(void *report);
// Atomically republishes the report pointer for double-buffered producers; unlike
// wiimote_emulator() this does not redo connection/SSP setup.
void wiimote_emulator_update_report(void *report);
void wiimote_emulator_shutdown(void);

#endif // _WIIMOTE_BTSTACK_H_
