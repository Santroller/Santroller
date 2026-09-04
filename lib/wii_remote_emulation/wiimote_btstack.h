#ifndef _WIIMOTE_BTSTACK_H_
#define _WIIMOTE_BTSTACK_H_

void wiimote_emulator_set_led(void (*led_on)(), void (*led_off)());

void wiimote_emulator(void *report);
// Atomically republishes the report pointer for double-buffered producers; unlike
// wiimote_emulator() this does not redo connection/SSP setup.
void wiimote_emulator_update_report(void *report);
void wiimote_emulator_shutdown(void);

#endif // _WIIMOTE_BTSTACK_H_
