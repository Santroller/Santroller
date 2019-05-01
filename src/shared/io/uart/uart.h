#pragma once

#if defined(UBRR1)
#  define UBRR UBRR1
#  define UCSRB UCSR1B
#  define TXEN TXEN1
#  define RXEN RXEN1
#  define RXCIE RXCIE1
#  define UDRIE UDRIE1
#  define UCSRC UCSR1C
#  define UCSZ0 UCSZ10
#  define UCSZ1 UCSZ11
#  define UCSRA UCSR1A
#  define UDRE UDRE1
#  define UDR UDR1
#elif defined(UBRR0)
#  define UBRR UBRR0
#  define UCSRB UCSR0B
#  define TXEN TXEN0
#  define RXEN RXEN0
#  define RXCIE RXCIE0
#  define UDRIE UDRIE0
#  define UCSRC UCSR0C
#  define UCSZ0 UCSZ00
#  define UCSZ1 UCSZ01
#  define UCSRA UCSR0A
#  define UDRE UDRE0
#  define UDR UDRE0
#endif
#ifdef __cplusplus
extern "C" {
#endif
void uart_init(void);
#ifdef __cplusplus
}
#endif