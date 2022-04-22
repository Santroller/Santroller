#pragma once
#include <stdint.h>

#define USBtoUSART_ReadPtr GPIOR0  // to use cbi()
#define USARTtoUSB_WritePtr GPIOR1

extern volatile uint8_t USBtoUSART_WritePtr;
extern volatile uint8_t USARTtoUSB_ReadPtr;

#define USART2USB_BUFLEN 128  // 0xFF - 8bit
#define USB2USART_BUFLEN 64   // 0x7F - 7bit

#define INIT_TMP_SERIAL_TO_USB(tmp_ptr)                                                \
    asm(                                                                               \
        "lds %A[tmp], %[readPtr]\n\t"       /* (1) Copy read pointer into lower byte*/ \
        /* Outputs */                                                                  \
        : [tmp] "=&e"(tmp_ptr)              /* Pointer register, output only*/         \
        /* Inputs */                                                                   \
        : [readPtr] "m"(USARTtoUSB_ReadPtr) /* Memory location */                      \
    )

#define READ_BYTE_FROM_BUF(data_ptr, tmp_ptr)                                        \
    asm(                                                                             \
        "ldi %B[tmp] , 0x01\n\t"     /* (1) Force high byte to 0x01 */               \
        "ld %[data] , %a[tmp] +\n\t" /* (2) Load next data byte, wraps around 255 */ \
        /* Outputs */                                                                \
        : [data] "=&r"(data_ptr),    /* Output only */                               \
          [tmp] "=e"(tmp_ptr)        /* Input and output */                          \
        /* Inputs */                                                                 \
        : "1"(tmp_ptr)                                                               \
    )

#define INIT_TMP_USB_TO_SERIAL(tmp_ptr)                                                          \
    asm(                                                                                         \
        "ldi %B[tmp], 0x02\n\t"               /* (1) Force high byte to 0x200 */                 \
        "lds %A[tmp], %[writePtr]\n\t"        /* (1) Load USBtoUSART_WritePtr into low byte */   \
        /* Outputs */                                                                            \
        : [tmp] "=&e"(tmp_ptr)                /* Pointer register, output only*/                 \
        /* Inputs */                                                                             \
        : [writePtr] "m"(USBtoUSART_WritePtr) /* Memory location */                              \
    )

#define WRITE_BYTE_TO_BUF(data_ptr, tmp_ptr)                                         \
    asm(                                                                             \
        "st %a[tmp]+, %[data]\n\t"     /* (2) Save byte in buffer and increment */   \
        "andi %A[tmp], 0x7F\n\t"       /* (1) wrap around pointer, 128 bytes */      \
        /* Outputs */                                                                \
        : [tmp] "=e"(tmp_ptr)          /* Input and output */                        \
        /* Inputs */                                                                 \
        : "0"(tmp_ptr),                                                              \
          [data] "r"(data_ptr)                                                       \
    )

#define WRITE_ARRAY_TO_BUF(tmp_ptr, buf_ptr, length) \
    {                                                \
        uint8_t len = length;                        \
        uint8_t* buf = (uint8_t*)buf_ptr;            \
        do {                                         \
            register uint8_t data;                   \
            data = *(buf++);                         \
            WRITE_BYTE_TO_BUF(data, tmp);            \
        } while (--len);                             \
    }

#define COMPLETE_WRITE(tmp_ptr)           \
    USBtoUSART_WritePtr = tmp_ptr & 0xFF; \
    UCSR1B = (_BV(RXCIE1) | _BV(TXEN1) | _BV(RXEN1) | _BV(UDRIE1))
