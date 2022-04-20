#pragma once

// This include needs to be at the top
#include "LUFAConfig.h"
// And then this one
#include <LUFA.h>

#define USBtoUSART_ReadPtr GPIOR0  // to use cbi()
#define USARTtoUSB_WritePtr GPIOR1

extern volatile uint8_t USBtoUSART_WritePtr;
extern volatile uint8_t USARTtoUSB_ReadPtr;

#define USART2USB_BUFLEN 128  // 0xFF - 8bit
#define USB2USART_BUFLEN 64   // 0x7F - 7bit

#define INIT_TMP_BUF(tmp_ptr)                                                          \
    asm(                                                                               \
        "lds %A[tmp], %[readPtr]\n\t"       /* (1) Copy read pointer into lower byte*/ \
        /* Outputs */                                                                  \
        : [tmp] "=&e"(tmp_ptr)              /* Pointer register, output only*/         \
        /* Inputs */                                                                   \
        : [readPtr] "m"(USARTtoUSB_ReadPtr) /* Memory location */                      \
    )

#define READ_BYTE_FROM_BUF(tmp_ptr)                                                  \
    register uint8_t data;                                                           \
    asm(                                                                             \
        "ldi %B[tmp] , 0x01\n\t"     /* (1) Force high byte to 0x01 */               \
        "ld %[data] , %a[tmp] +\n\t" /* (2) Load next data byte, wraps around 255 */ \
        /* Outputs */                                                                \
        : [data] "=&r"(data),        /* Output only */                               \
          [tmp] "=e"(tmp_ptr)        /* Input and output */                          \
        /* Inputs */                                                                 \
        : "1"(tmp_ptr))

uint8_t Endpoint_Write_Control_Buffer_LE(const void* const Buffer,
                                         uint16_t Length);

uint8_t Endpoint_Write_Buffer_LE(const void* const Buffer,
                                 uint16_t Length,
                                 uint16_t* const BytesProcessed);
void writeData(const void* data, uint8_t len);