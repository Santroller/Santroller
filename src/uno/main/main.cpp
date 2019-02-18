#include "../../shared/Controller.h"
#include "../../shared/twi/I2Cdev.h"
#include "../../shared/util.h"
#include "../../shared/wii/WiiExtension.h"
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/sfr_defs.h>
#include <util/delay.h>
#include "uart.h"

WiiExtension controller;

size_t current_index = 0;
USB_JoystickReport_Data_t data;
int main() {
  sei();
  uart_init();
  controller.init();
  UCSR0B = _BV(TXEN0) | _BV(UDRIE0);
  for (;;) {
    controller.read_controller(&data);
  }
}
ISR(USART_UDRE_vect) {
  uint8_t a = UDR0;
  if (current_index < 2) {
    UDR0 = current_index == 0 ? 'm' : 'a';
  } else {
    UDR0 = ((uint8_t *)&data)[current_index - 2];
  }
  current_index++;
  if (current_index >= sizeof(data) + 2) {
    current_index = 0;
  }
}
