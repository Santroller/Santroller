#pragma once
#include <avr/io.h>
// To make things compatible with all uno/mega processors, we redefine all the
// serial definitions to a standard that doesnt define specific ports. The USB
// processors use port 1, while the main processors use serial port 0
#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega2560__) ||              \
    defined(__AVR_ATmega1280__)
#  define UBRR UBRR0
#  define UDR UDR0
#  define UCSRA UCSR0A
#  define UCSRB UCSR0B
#  define UCSRC UCSR0C
#  define UCSZ0 UCSZ00
#  define UCSZ1 UCSZ01
#  define UDRE UDRE0
#  define UDRIE UDRIE0
#  define TXC TXC0
#  define RXC RXC0
#  define TXEN TXEN0
#  define RXEN RXEN0
#  define RXCIE RXCIE0
#  define U2X U2X0
#  if defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__)
#    define USART_RX_vect USART0_RX_vect
#    define USART_UDRE_vect USART0_UDRE_vect
#  endif
#elif defined(__AVR_AT90USB82__) || defined(__AVR_ATmega16U2__)
#  define UBRR UBRR1
#  define UDR UDR1
#  define UCSRA UCSR1A
#  define UCSRB UCSR1B
#  define UCSRC UCSR1C
#  define UCSZ0 UCSZ10
#  define UCSZ1 UCSZ11
#  define UDRE UDRE1
#  define UDRIE UDRIE1
#  define TXC TXC1
#  define RXC RXC1
#  define TXEN TXEN1
#  define RXEN RXEN1
#  define RXCIE RXCIE1
#  define U2X U2X1
#  define USART_RX_vect USART1_RX_vect
#  define USART_UDRE_vect USART1_UDRE_vect
#endif
#include "util/util.h"
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

/** Macro for calculating the baud value from a given baud rate when the \c U2X
 * (double speed) bit is not set.
 *
 *  \param[in] Baud  Target serial UART baud rate.
 *
 *  \return Closest UBRR register value for the given UART frequency.
 */
#define SERIAL_UBBRVAL(Baud) ((((F_CPU / 16) + (Baud / 2)) / (Baud)) - 1)

/** Macro for calculating the baud value from a given baud rate when the \c U2X
 * (double speed) bit is set.
 *
 *  \param[in] Baud  Target serial UART baud rate.
 *
 *  \return Closest UBRR register value for the given UART frequency.
 */
#define SERIAL_2X_UBBRVAL(Baud) ((((F_CPU / 8) + (Baud / 2)) / (Baud)) - 1)

/* USBtoUSART_WritePtr needs to be visible to ISR. */
/* USARTtoUSB_ReadPtr needs to be visible to CDC LineEncoding Event. */
static volatile uint8_t USBtoUSART_WritePtr = 0;
static volatile uint8_t USARTtoUSB_ReadPtr = 0;
static inline void Serial_InitInterrupt(const uint32_t BaudRate,
                                        const bool DoubleSpeed) {
  UBRR = (DoubleSpeed ? SERIAL_2X_UBBRVAL(BaudRate) : SERIAL_UBBRVAL(BaudRate));

  UCSRC = ((1 << UCSZ1) | (1 << UCSZ0));
  UCSRA = (DoubleSpeed ? (1 << U2X) : 0);
  UCSRB = ((1 << TXEN) | (1 << RXCIE) | (1 << RXEN));

  DDRD |= (1 << 3);
  PORTD |= (1 << 2);
  USBtoUSART_ReadPtr = 0;
  USBtoUSART_WritePtr = 0;
  USARTtoUSB_ReadPtr = 0;
  USARTtoUSB_WritePtr = 0;
}

void writeData(const uint8_t *buf, uint8_t len) {

  //================================================================================
  // USBtoUSART
  //================================================================================

  // Check how much free space the USBtoUSART buffer has
  uint8_t USBtoUSART_free =
      (USB2USART_BUFLEN - 1) -
      ((USBtoUSART_WritePtr - USBtoUSART_ReadPtr) & (USB2USART_BUFLEN - 1));

  // Read new data from the USB host if we still have space in the buffer
  while (len > USBtoUSART_free) {}
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
  UCSRB = (_BV(RXCIE) | _BV(TXEN) | _BV(RXEN) | _BV(UDRIE));
}

/** ISR to manage the reception of data from the serial port, placing received
 * bytes into a circular buffer for later transmission to the host.
 */
ISR(USART_RX_vect, ISR_NAKED) {
  // This ISR doesnt change SREG. Whoa.
  asm volatile(
      "lds r3, %[UDR1_Reg]\n\t" // (1) Load new Serial byte (UDR1) into r3
      "movw r4, r30\n\t"        // (1) Backup Z pointer (r30 -> r4, r31 -> r5)
      "in r30, %[writePointer]\n\t" // (1) Load USARTtoUSB write buffer 8 bit
                                    // pointer to lower Z pointer
      "ldi r31, 0x01\n\t"           // (1) Set higher Z pointer to 0x01
      "st Z+, r3\n\t" // (2) Save UDR1 in Z pointer (USARTtoUSB write buffer)
                      // and increment
      "out %[writePointer], r30\n\t" // (1) Save back new USARTtoUSB buffer
                                     // pointer location
      "movw r30, r4\n\t"             // (1) Restore backuped Z pointer
      "reti\n\t"                     // (4) Exit ISR

      // Inputs:
      ::[UDR1_Reg] "m"(UDR), // Memory location of UDR1
      [writePointer] "I"(_SFR_IO_ADDR(
          USARTtoUSB_WritePtr)) // 8 bit pointer to USARTtoUSB write buffer
  );
}

ISR(USART_UDRE_vect, ISR_NAKED) {
  // Another SREG-less ISR.
  asm volatile(
      "movw r4, r30\n\t" // (1) Backup Z pointer (r30 -> r4, r31 -> r5)
      "in r30, %[readPointer]\n\t" // (1) Load USBtoUSART read buffer 8 bit
                                   // pointer to lower Z pointer
      "ldi r31, 0x02\n\t"          // (1) Set higher Z pointer to 0x02
      "ld r3, Z+\n\t" // (2) Load next byte from USBtoUSART buffer into r3
      "sts %[UDR1_Reg], r3\n\t"     // (2) Save r3 (next byte) in UDR1
      "out %[readPointer], r30\n\t" // (1) Save back new USBtoUSART read
                                    // buffer pointer location
      "cbi %[readPointer], 7\n\t"   // (2) Wrap around for 128 bytes
      //     smart after-the-fact andi 0x7F without using SREG
      "movw r30, r4\n\t"            // (1) Restore backuped Z pointer
      "in r2, %[readPointer]\n\t"   // (1) Load USBtoUSART read buffer 8 bit
                                    // pointer to r2
      "lds r3, %[writePointer]\n\t" // (1) Load USBtoUSART write buffer to r3
      "cpse r2, r3\n\t"   // (1/2) Check if USBtoUSART read buffer == USBtoUSART
                          // write buffer
      "reti\n\t"          // (4) They are not equal, more bytes coming soon!
      "ldi r30, 0x98\n\t" // (1) Set r30 temporary to new UCSR1B setting
                          // ((1<<RXCIE1) | (1 << RXEN1) | (1 << TXEN1))
      //     ldi needs an upper register, restore Z once more afterwards
      "sts %[UCSR1B_Reg], r30\n\t" // (2) Turn off this interrupt (UDRIE1),
                                   // all bytes sent
      "movw r30, r4\n\t"           // (1) Restore backuped Z pointer again (was
                                   // overwritten again above)
      "reti\n\t"                   // (4) Exit ISR

      // Inputs:
      ::[UDR1_Reg] "m"(UDR), // Memory location of UDR1
      [readPointer] "I"(_SFR_IO_ADDR(
          USBtoUSART_ReadPtr)), // 7 bit pointer to USBtoUSART read buffer
      [writePointer] "m"(
          USBtoUSART_WritePtr), // 7 bit pointer to USBtoUSART write buffer
      [UCSR1B_Reg] "m"(UCSRB)   // Memory location of UCSR1B
  );
}