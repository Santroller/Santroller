#ifndef _WIIMOTE_BTSTACK_H_
#define _WIIMOTE_BTSTACK_H_

void wiimote_emulator_set_led(void (*led_on)(), void (*led_off)());

void wiimote_emulator(void *report);
void wiimote_emulator_shutdown(void);

#endif // _WIIMOTE_BTSTACK_H_
