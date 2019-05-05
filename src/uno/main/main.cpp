#include "../../shared/controller/input/InputHandler.h"
#include "../../shared/io/uart/uart.h"
#include "../../shared/util.h"
#include "../../shared/sharedmain.h"
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/sfr_defs.h>
#include <avr/wdt.h>
#include <util/delay.h>
extern "C" {
#include "pins.h"
}

InputHandler controller;
Main sharedMain;

size_t controller_index = 0;
bool done = false;
int main() {
  sharedMain.main();
  UBRR0 = 6;
  UCSR0B = _BV(TXEN0) | _BV(UDRIE0);
  UCSR0C = _BV(UCSZ00) | _BV(UCSZ01);
  sei();
  controller.init();
  while (true) {
    controller.process();
  }
}
ISR(USART_UDRE_vect) {
  if (controller_index < 2) {
    UDR0 = controller_index == 0 ? 'm' : 'a';
  } else {
    UDR0 = ((uint8_t *)&controller.controller)[controller_index - 2];
  }
  controller_index++;
  if (controller_index >= sizeof(Controller) + 2) {
    controller_index = 0;
  }
}