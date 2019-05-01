#include <avr/io.h>
#include "uart.h"
void uart_init(void) {
  // Set baud rate
  UBRR = 6;
  //   UCSR0A = _BV(U2X0);
  // Enable transmitter, enable transmit interrupt, enable receiver interrupt
  UCSRB = _BV(TXEN) | _BV(RXEN) | _BV(RXCIE) | _BV(UDRIE);
  // Set frame format: 8data, 1stop bit
  UCSRC = _BV(UCSZ0) | _BV(UCSZ1);
}