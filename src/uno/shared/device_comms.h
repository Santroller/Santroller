#pragma once
#ifndef UDR1
#  define UBRR1 UBRR0
#  define UDR1 UDR0
#  define UCSR1A UCSR0A
#  define UCSR1B UCSR0B
#  define UCSR1C UCSR0C
#  define UCSZ10 UCSZ00
#  define UCSZ11 UCSZ01
#  define UDRE1 UDRE0
#  define UDRIE1 UDRIE0
#  define TXC1 TXC0
#  define RXC1 RXC0
#  define TXEN1 TXEN0
#  define RXEN1 RXEN0
#  define RXCIE1 RXCIE0
#  define U2X1 U2X0
#  ifdef USART_RX_vect
#    define USART1_RX_vect USART_RX_vect
#  else
#    define USART1_RX_vect USART0_RX_vect
#  endif
#  ifdef USART_UDRE_vect
#    define USART1_UDRE_vect USART_UDRE_vect
#  else
#    define USART1_UDRE_vect USART0_UDRE_vect
#  endif
#endif
#include "util/util.h"
#include <LUFA/Drivers/Peripheral/Serial.h>
#define BAUD 1000000
#define FRAME_START_FEATURE_READ 0x7d
#define FRAME_START_FEATURE_WRITE 0x7e
#define FRAME_START_WRITE 0x78
#define FRAME_DONE 0x77

/* NOTE: Using Linker Magic,
 * - Reserved 256 bytes from start of RAM at 0x100 for UART RX Buffer
 * so we can use 256-byte aligned addresssing.
 * - Also 128 bytes from 0x200 for UART TX buffer, same addressing.
 * normal RAM data starts at 0x280, see offset in makefile*/

#define USART2USB_BUFLEN 256 // 0xFF - 8bit
#define USB2USART_BUFLEN 128 // 0x7F - 7bit

// USB-Serial buffer pointers are saved in GPIORn
// for better access (e.g. cbi) in ISRs
// This has nothing to do with r0 and r1!
// GPIORn – General Purpose I/O Register are located in RAM.
// Make sure to set DEVICE_STATE_AS_GPIOR to 2 in the Lufa config.
// They are initialied in the CDC LineEncoding Event
#define USBtoUSART_ReadPtr GPIOR0 // to use cbi()
#define USARTtoUSB_WritePtr GPIOR1

/* USBtoUSART_WritePtr needs to be visible to ISR. */
/* USARTtoUSB_ReadPtr needs to be visible to CDC LineEncoding Event. */
static volatile uint8_t USBtoUSART_WritePtr = 0;
static volatile uint8_t USARTtoUSB_ReadPtr = 0;
static inline void Serial_InitInterrupt(const uint32_t BaudRate,
                                        const bool DoubleSpeed) {
  UBRR1 =
      (DoubleSpeed ? SERIAL_2X_UBBRVAL(BaudRate) : SERIAL_UBBRVAL(BaudRate));

  UCSR1C = ((1 << UCSZ11) | (1 << UCSZ10));
  UCSR1A = (DoubleSpeed ? (1 << U2X1) : 0);
  UCSR1B = ((1 << TXEN1) | (1 << RXCIE1) | (1 << RXEN1));

  DDRD |= (1 << 3);
  PORTD |= (1 << 2);
}

void writeData(uint8_t *buf, uint8_t len) {

  //================================================================================
  // USBtoUSART
  //================================================================================

  // Check how much free space the USBtoUSART buffer has
  uint8_t USBtoUSART_free =
      (USB2USART_BUFLEN - 1) -
      ((USBtoUSART_WritePtr - USBtoUSART_ReadPtr) & (USB2USART_BUFLEN - 1));

  // Read new data from the USB host if we still have space in the buffer
  if (len <= USBtoUSART_free) {
    // Prepare temporary pointer
    uint16_t tmp;                      // = 0x200 | USBtoUSART_WritePtr;
    asm("ldi %B[tmp], 0x02\n\t"        // (1) Force high byte to 0x200
        "lds %A[tmp], %[writePtr]\n\t" // (1) Load USBtoUSART_WritePtr into
                                       // low byte
        // Outputs
        : [tmp] "=&e"(tmp) // Pointer register, output only
        // Inputs
        : [writePtr] "m"(USBtoUSART_WritePtr) // Memory location
    );

    // Save USB bank into our USBtoUSART ringbuffer
    do {
      register uint8_t data;
      data = *(buf++);
      asm("st %a[tmp]+, %[data]\n\t" // (2) Save byte in buffer and
                                     // increment
          "andi %A[tmp], 0x7F\n\t"   // (1) Wrap around pointer, 128 bytes
          // Outputs
          : [tmp] "=e"(tmp) // Input and output
          // Inputs
          : "0"(tmp), [data] "r"(data));
    } while (--len);

    // Save back new pointer position
    // Just save the lower byte of the pointer
    USBtoUSART_WritePtr = tmp & 0xFF;

    // Enable USART again to flush the buffer
    UCSR1B = (_BV(RXCIE1) | _BV(TXEN1) | _BV(RXEN1) | _BV(UDRIE1));

    // Force Leds to turn on
    USBtoUSART_free = 0;
  }
}