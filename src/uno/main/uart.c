#include <avr/io.h>

void uart_init(void) {
     // Set baud rate
  UBRR0 = 16;
//   UCSR0A = _BV(U2X0);
  // Enable transmitter, enable transmit interrupt
  UCSR0B = _BV(TXEN0);
  // Set frame format: 8data, 1stop bit
  UCSR0C = _BV(UCSZ00) | _BV(UCSZ01);
}

void uart_putchar(char c) {
    loop_until_bit_is_set(UCSR0A, UDRE0);
    UDR0 = c;
}

void uart_putstr(const char* c) {
    for (uint8_t i =0; c[i] != '\0'; i++) {
        uart_putchar(c[i]);
    }
}