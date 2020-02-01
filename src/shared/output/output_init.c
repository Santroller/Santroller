#include "output_init.h"


void output_init(void) {      
  if (config.main.sub_type >= KEYBOARD_SUBTYPE) {
    if (config.main.sub_type == KEYBOARD_SUBTYPE) {
      keyboard_init();
    } else {
      ps3_init();
    }
  } else {
    xinput_init();
  }
  USB_Init();
  sei();
}