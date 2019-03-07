#include <stdlib.h>
#include "../../shared/controller/Controller.h"
#include "../../shared/controller/output/OutputHandler.h"
#include "../../shared/bootloader/Bootloader.h"
#include "../../shared/util.h"
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <util/delay.h>

#define CPU_PRESCALE(n) (CLKPR = 0x80, CLKPR = (n))

uint8_t current_control;
Controller controller;
OutputHandler out;
ISR(USART1_RX_vect) {
  char data = UDR1;
  switch (current_control) {
  case 0:
    if (data == 'm') {
      current_control++;
    }
    break;
  case 1:
    if (data == 'a')
      current_control++;
    else
      current_control = 0;
    break;
  default:
    ((uint8_t *)&controller)[current_control - 2] = data;
    current_control++;
    if (current_control == sizeof(Controller) + 2) {
      out.process(&controller);
      current_control = 0;
    }
  }
}

void USART_Init() {
  // Set baud rate
  UBRR1 = 6;
  // UCSR1A = _BV(U2X1);
  // Enable receiver and interrupt
  UCSR1B = _BV(RXEN1) | _BV(RXCIE1);
  // Set frame format: 8data, 1stop bit
  UCSR1C = _BV(UCSZ10) | _BV(UCSZ11);
}

int main(void) {

  USART_Init();
  // Set clock @ 16Mhz
  CPU_PRESCALE(0);

  out.init();
  while (true) {};
}