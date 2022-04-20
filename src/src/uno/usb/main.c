/**
 * You should have a LUFAConfig.h for this to work.
 */
#include "LUFAConfig.h"

/**
 * Include LUFA.h after LUFAConfig.h
 */
#include <LUFA.h>
#include <LUFA/LUFA/Drivers/Board/Board.h>
#include <LUFA/LUFA/Drivers/Board/LEDs.h>
#include <LUFA/LUFA/Drivers/Peripheral/Serial.h>
#include <LUFA/LUFA/Drivers/USB/Class/CDCClass.h>
#include <LUFA/LUFA/Drivers/USB/USB.h>
#include <avr/boot.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/power.h>
#include <avr/wdt.h>

#include "commands.h"
#include "defines.h"
#include "endpoints.h"
#include "packets.h"
#include "reboot.h"
#include "serial_descriptors.h"
#define JUMP 0xDE00
#define BUFFER_NEARLY_FULL 96
static CDC_LineEncoding_t LineEncoding = {.BaudRateBPS = 0,
                                          .CharFormat = CDC_LINEENCODING_OneStopBit,
                                          .ParityType = CDC_PARITY_None,
                                          .DataBits = 8};

#define USBtoUSART_ReadPtr GPIOR0  // to use cbi()
#define USARTtoUSB_WritePtr GPIOR1

/* USBtoUSART_WritePtr needs to be visible to ISR. */
/* USARTtoUSB_ReadPtr needs to be visible to CDC LineEncoding Event. */
static volatile uint8_t USBtoUSART_WritePtr = 0;
static volatile uint8_t USARTtoUSB_ReadPtr = 0;
#define USART2USB_BUFLEN 128  // 0xFF - 8bit
#define USB2USART_BUFLEN 64   // 0x7F - 7bit
// if bootloaderState is set to JUMP, then the arduino will jump to bootloader
// mode after the next watchdog reset
volatile uint16_t bootloaderState __attribute__((section(".noinit")));

volatile bool ready = false;
bool serial = false;

void writeData(const void* data, uint8_t len) {
    uint8_t* buf = (uint8_t*)data;
    //================================================================================
    // USBtoUSART
    //================================================================================

    // Prepare temporary pointer
    uint16_t tmp;                       // = 0x200 | USBtoUSART_WritePtr;
    asm("ldi %B[tmp], 0x02\n\t"         // (1) Force high byte to 0x200
        "lds %A[tmp], %[writePtr]\n\t"  // (1) Load USBtoUSART_WritePtr into
                                        // low byte
        // Outputs
        : [tmp] "=&e"(tmp)  // Pointer register, output only
        // Inputs
        : [writePtr] "m"(USBtoUSART_WritePtr)  // Memory location
    );

    // Save USB bank into our USBtoUSART ringbuffer
    do {
        register uint8_t data;
        data = *(buf++);
        asm("st %a[tmp]+, %[data]\n\t"  // (2) Save byte in buffer and
                                        // increment
            "andi %A[tmp], 0x7F\n\t"    // (1) Wrap around pointer, 128 bytes
            // Outputs
            : [tmp] "=e"(tmp)  // Input and output
            // Inputs
            : "0"(tmp), [data] "r"(data));
    } while (--len);

    // Save back new pointer position
    // Just save the lower byte of the pointer
    USBtoUSART_WritePtr = tmp & 0xFF;

    // Enable USART again to flush the buffer
    UCSR1B = (_BV(RXCIE1) | _BV(TXEN1) | _BV(RXEN1) | _BV(UDRIE1));
}

#define TEMPLATE_FUNC_NAME Endpoint_Write_Control_Buffer_LE
#define TEMPLATE_BUFFER_OFFSET(Length) 0
#define TEMPLATE_BUFFER_MOVE(BufferPtr, Amount)
#define TEMPLATE_TRANSFER_BYTE(BufferPtr)   \
    register uint8_t data;                  \
    asm("ldi %B[tmp] , 0x01\n\t"            \
        "ld %[data] , %a[tmp] +\n\t"        \
        : [data] "=&r"(data),               \
          [tmp] "=e"(*(uint16_t*)BufferPtr) \
        : "1"(*(uint16_t*)BufferPtr));      \
    Endpoint_Write_8(data)
#include "LUFA/LUFA/Drivers/USB/Core/AVR8/Template/Template_Endpoint_Control_W.c"

#define TEMPLATE_FUNC_NAME Endpoint_Write_Buffer_LE
#define TEMPLATE_BUFFER_TYPE const void*
#define TEMPLATE_CLEAR_ENDPOINT() Endpoint_ClearIN()
#define TEMPLATE_BUFFER_OFFSET(Length) 0
#define TEMPLATE_BUFFER_MOVE(BufferPtr, Amount)
#define TEMPLATE_TRANSFER_BYTE(BufferPtr)   \
    register uint8_t data;                  \
    asm("ldi %B[tmp] , 0x01\n\t"            \
        "ld %[data] , %a[tmp] +\n\t"        \
        : [data] "=&r"(data),               \
          [tmp] "=e"(*(uint16_t*)BufferPtr) \
        : "1"(*(uint16_t*)BufferPtr));      \
    Endpoint_Write_8(data)
#include "LUFA/LUFA/Drivers/USB/Core/AVR8/Template/Template_Endpoint_RW.c"
bool waitingForData = true;

void readControlData() {
    uint8_t packetCount = 0;
    // Prepare temporary pointer
    uint16_t tmp;  // = 0x100 | USARTtoUSBReadPtr
    asm(
        // Do not initialize high byte, it will be done in first loop
        // below.
        "lds %A[tmp], %[readPtr]\n\t"  // (1) Copy read pointer into
                                       // lower byte
        // Outputs
        : [tmp] "=&e"(tmp)  // Pointer register, output only
        // Inputs
        : [readPtr] "m"(USARTtoUSB_ReadPtr)  // Memory location
    );
    uint8_t state = STATE_NO_PACKET;
    while (true) {
        // Wait for a byte
        while (USARTtoUSB_WritePtr - (tmp & 0xff) == 0) {
        }
        register uint8_t data;
        asm("ldi %B[tmp] , 0x01\n\t"      // (1) Force high byte to 0x01
            "ld %[data] , %a[tmp] +\n\t"  // (2) Load next data byte, wraps
                                          // around 255
            // Outputs
            : [data] "=&r"(data),  // Output only
              [tmp] "=e"(tmp)      // Input and output
            // Inputs
            : "1"(tmp));
        if (state == STATE_NO_PACKET && data == VALID_PACKET) {
            state = STATE_READ_PACKET_TYPE;
        } else if (state == STATE_READ_PACKET_TYPE) {
            if (data == DEVICE_ID) {
                state = STATE_READ_DEVICE_ID;
            } else if (data == DESCRIPTOR_ID || data == CONTROL_REQUEST_ID) {
                state = STATE_READ_LENGTH;
            } else if (data == CONTROL_REQUEST_INVALID_ID) {
                state = STATE_READ_INVALID_CONTROL_REQUEST_LENGTH;
            }
        } else if (state == STATE_READ_INVALID_CONTROL_REQUEST_LENGTH) {
            return;
        } else if (state == STATE_READ_LENGTH) {
            packetCount = data;
            break;
        } else if (state == STATE_READ_DEVICE_ID) {
            uint8_t type = EP_TYPE_INTERRUPT;
            uint8_t epsize = 0x20;
            uint8_t consoleType = data;
            if (consoleType == XBOX360 || consoleType == PC_XINPUT) {
                epsize = 0x18;
            }
            if (consoleType == MIDI) {
                type = EP_TYPE_BULK;
            }
            Endpoint_ConfigureEndpoint(DEVICE_EPADDR_IN, type, epsize, 1);
            Endpoint_ConfigureEndpoint(DEVICE_EPADDR_OUT, type, 0x08, 2);
            USARTtoUSB_ReadPtr = tmp & 0xFF;
            return;
        }
    }
    // Now wait to read the whole packet
    while (USARTtoUSB_WritePtr - (tmp & 0xff) < packetCount) {
    }
    Endpoint_ClearSETUP();
    Endpoint_Write_Control_Buffer_LE(&tmp, packetCount);
    // Save new pointer position
    USARTtoUSB_ReadPtr = 0;
    USARTtoUSB_WritePtr = 0;
}
void readSerialData() {
    // Prepare temporary pointer
    uint16_t tmp;  // = 0x100 | USARTtoUSBReadPtr
    asm(
        // Do not initialize high byte, it will be done in first loop
        // below.
        "lds %A[tmp], %[readPtr]\n\t"  // (1) Copy read pointer into
                                       // lower byte
        // Outputs
        : [tmp] "=&e"(tmp)  // Pointer register, output only
        // Inputs
        : [readPtr] "m"(USARTtoUSB_ReadPtr)  // Memory location
    );
    uint8_t txcount;
    uint8_t count = USARTtoUSB_WritePtr - USARTtoUSB_ReadPtr;
    // Check if the UART receive buffer flush timer has expired or the buffer is nearly full
    if (!((TIFR0 & (1 << TOV0)) || (count >= (SERIAL_TX_SIZE - 1)))) {
        return;
    }
    TIFR0 = (1 << TOV0);
    txcount = SERIAL_TX_SIZE - 1;
    if (txcount > count) {
        txcount = count;
    }
    Endpoint_Write_Buffer_LE(&tmp, txcount, NULL);
    // Save new pointer position
    USARTtoUSB_ReadPtr = tmp & 0xFF;
    Endpoint_ClearIN();
}
void readEndpointData() {
    // Prepare temporary pointer
    uint16_t tmp;  // = 0x100 | USARTtoUSBReadPtr
    asm(
        // Do not initialize high byte, it will be done in first loop
        // below.
        "lds %A[tmp], %[readPtr]\n\t"  // (1) Copy read pointer into
                                       // lower byte
        // Outputs
        : [tmp] "=&e"(tmp)  // Pointer register, output only
        // Inputs
        : [readPtr] "m"(USARTtoUSB_ReadPtr)  // Memory location
    );
    uint8_t txcount;
    uint8_t state = STATE_NO_PACKET;
    while (true) {
        // Wait for a byte
        while (USARTtoUSB_WritePtr - (tmp & 0xff) == 0) {
        }
        register uint8_t data;
        asm("ldi %B[tmp] , 0x01\n\t"      // (1) Force high byte to 0x01
            "ld %[data] , %a[tmp] +\n\t"  // (2) Load next data byte, wraps
                                          // around 255
            // Outputs
            : [data] "=&r"(data),  // Output only
              [tmp] "=e"(tmp)      // Input and output
            // Inputs
            : "1"(tmp));
        if (state == STATE_NO_PACKET && data == VALID_PACKET) {
            state = STATE_READ_PACKET_TYPE;
        } else if (state == STATE_READ_PACKET_TYPE) {
            if (data == CONTROLLER_DATA_REQUEST_ID) {
                state = STATE_READ_LENGTH;
            } else if (data == CONTROLLER_DATA_REBOOT_ID) {
                reboot();
            }
        } else if (state == STATE_READ_LENGTH) {
            txcount = data;
            break;
        }
    }
    // Now wait to read the whole packet
    while (USARTtoUSB_WritePtr - (tmp & 0xff) < txcount) {
    }
    Endpoint_Write_Buffer_LE(&tmp, txcount, NULL);
    // Save new pointer position
    USARTtoUSB_ReadPtr = tmp & 0xFF;
    Endpoint_ClearIN();
}
int main(void) {
    if (bootloaderState == (JUMP | COMMAND_JUMP_BOOTLOADER)) {
        // We don't want to jump again after the bootloader returns control flow to
        // us
        bootloaderState = 0;
        asm volatile("jmp 0x1000");
    } else if (bootloaderState == (JUMP | COMMAND_JUMP_BOOTLOADER_UNO)) {
        serial = true;
    }
    bootloaderState = 0;

    /* Disable watchdog if enabled by bootloader/fuses */
    MCUSR &= ~(1 << WDRF);
    wdt_disable();

    clock_prescale_set(clock_div_1);

    UCSR1C = ((1 << UCSZ11) | (1 << UCSZ10));
    UCSR1A = (1 << U2X1);
    UCSR1B = ((1 << TXEN1) | (1 << RXEN1));
    UBRR1 = SERIAL_2X_UBBRVAL(BAUD);

    DDRD |= (1 << 3);
    PORTD |= (1 << 2);
    AVR_RESET_LINE_DDR |= AVR_RESET_LINE_MASK;
    // Ensure the 328p is reset so we can also work from bootloader
    AVR_RESET_LINE_PORT |= AVR_RESET_LINE_MASK;
    _delay_ms(1);
    AVR_RESET_LINE_PORT &= ~AVR_RESET_LINE_MASK;
    _delay_ms(1);
    AVR_RESET_LINE_PORT |= AVR_RESET_LINE_MASK;
    sei();
    if (serial) {
        TCCR0B = (1 << CS02);
    } else {
        uint16_t count = 0;
        uint8_t count2 = 0;
        // Wait for the 328p / 1280 to boot (we receive a READY byte)
        // Since it is set in the isr, we need to treat it as volatile
        // If we don't get it in time, we can assume the 328p is missing its firmware and drop to the 328p programming mode
        while (!(UCSR1A & (1 << RXC1)) || UDR1 != READY) {
            count++;
            if (count > 0xFFFE) {
                count2++;
                count = 0;
            }
            if (count2 > 0x2F) {
                bootloaderState = (JUMP | COMMAND_JUMP_BOOTLOADER_UNO);
                reboot();
            }
        }
    }
    UCSR1B = ((1 << TXEN1) | (1 << RXCIE1) | (1 << RXEN1));
    USB_Init();
    if (serial) {
        while (true) {
            USB_USBTask();
            Endpoint_SelectEndpoint(DEVICE_EPADDR_IN);
            if (Endpoint_IsINReady()) {
                readSerialData();
            }
            //================================================================================
            // USBtoUSART
            //================================================================================
            Endpoint_SelectEndpoint(DEVICE_EPADDR_OUT);
            uint8_t countRX = 0;
            if (Endpoint_IsOUTReceived()) {
                // Check if we received any new bytes and if we still have space in the buffer
                countRX = Endpoint_BytesInEndpoint();

                // Acknowledge zero length packet and dont call any read functions
                if (!countRX)
                    Endpoint_ClearOUT();
            }
            uint8_t USBtoUSART_free = (USB2USART_BUFLEN - 1) - ((USBtoUSART_WritePtr - USBtoUSART_ReadPtr) & (USB2USART_BUFLEN - 1));
            if (countRX && countRX <= USBtoUSART_free) {
                while (countRX--) {
                    uint8_t byte = Endpoint_Read_8();
                    writeData(&byte, 1);
                }
                Endpoint_ClearOUT();
            }
        }
    }
    packet_header_t requestData = {
        VALID_PACKET, CONTROLLER_DATA_REQUEST_ID, sizeof(packet_header_t)};
    while (true) {
        USB_USBTask();
        if (waitingForData) {
            waitingForData = false;
            Endpoint_SelectEndpoint(DEVICE_EPADDR_IN);
            if (Endpoint_IsINReady()) {
                writeData(&requestData, sizeof(packet_header_t));
            }
        }
        Endpoint_SelectEndpoint(DEVICE_EPADDR_OUT);
        if (Endpoint_IsOUTReceived()) {
            data_transmit_packet_t packet;
            packet.header.magic = VALID_PACKET;
            packet.header.id = CONTROLLER_DATA_TRANSMIT_ID;
            packet.header.len = sizeof(data_transmit_packet_t) + Endpoint_BytesInEndpoint();
            writeData(&packet, sizeof(data_transmit_packet_t));
            /* Check to see if the packet contains data */
            if (Endpoint_IsReadWriteAllowed()) {
                uint8_t count = Endpoint_BytesInEndpoint();
                while (count--) {
                    uint8_t byte = Endpoint_Read_8();
                    writeData(&byte, 1);
                }
            }
            Endpoint_ClearOUT();
        }
    }
}

void EVENT_USB_Device_ControlRequest(void) {
    if (!(Endpoint_IsSETUPReceived())) return;
    if (USB_ControlRequest.bRequest == REQ_GetDescriptor && (USB_ControlRequest.bmRequestType == (REQDIR_DEVICETOHOST | REQTYPE_STANDARD | REQREC_DEVICE) ||
                                                             (USB_ControlRequest.bmRequestType == (REQDIR_DEVICETOHOST | REQTYPE_STANDARD | REQREC_INTERFACE)))) {
        return;
    }
    if (USB_ControlRequest.bRequest == REQ_SetAddress && USB_ControlRequest.bmRequestType == 0x00) {
        return;
    }
    if (serial) {
        if (USB_ControlRequest.bmRequestType == (REQDIR_HOSTTODEVICE | REQTYPE_CLASS | REQREC_INTERFACE)) {
            if (USB_ControlRequest.bRequest == CDC_REQ_SetLineEncoding) {
                Endpoint_ClearSETUP();

                /* Read the line coding data in from the host into the global struct */
                Endpoint_Read_Control_Stream_LE(&LineEncoding, sizeof(CDC_LineEncoding_t));
                Endpoint_ClearIN();
                if (LineEncoding.BaudRateBPS == 1200) {
                    bootloaderState = (JUMP | COMMAND_JUMP_BOOTLOADER);
                    reboot();
                } else if (LineEncoding.BaudRateBPS == 2400) {
                    bootloaderState = JUMP;
                    reboot();
                }
                /* Keep the TX line held high (idle) while the USART is reconfigured */
                PORTD |= (1 << 3);
                /* Must turn off USART before reconfiguring it, otherwise incorrect operation may occur */
                UCSR1B = 0;
                UCSR1A = 0;
                UCSR1C = 0;
                USBtoUSART_ReadPtr = 0;
                USBtoUSART_WritePtr = 0;
                USARTtoUSB_ReadPtr = 0;
                USARTtoUSB_WritePtr = 0;

                UBRR1 = SERIAL_2X_UBBRVAL(LineEncoding.BaudRateBPS);

                UCSR1C = ((1 << UCSZ11) | (1 << UCSZ10));
                UCSR1A = (1 << U2X1);
                UCSR1B = ((1 << RXCIE1) | (1 << TXEN1) | (1 << RXEN1));

                /* Release the TX line after the USART has been reconfigured */
                PORTD &= ~(1 << 3);
            } else if (USB_ControlRequest.bRequest == CDC_REQ_SetControlLineState) {
                Endpoint_ClearSETUP();
                Endpoint_ClearStatusStage();

                if (USB_ControlRequest.wValue & CDC_CONTROL_LINE_OUT_DTR) {
                    AVR_RESET_LINE_PORT &= ~AVR_RESET_LINE_MASK;
                } else {
                    AVR_RESET_LINE_PORT |= AVR_RESET_LINE_MASK;
                }
            }
        } else if (USB_ControlRequest.bmRequestType == (REQDIR_DEVICETOHOST | REQTYPE_CLASS | REQREC_INTERFACE) && USB_ControlRequest.bRequest == CDC_REQ_GetLineEncoding) {
            Endpoint_ClearSETUP();
            Endpoint_Write_Control_Stream_LE(&LineEncoding, sizeof(CDC_LineEncoding_t));
            Endpoint_ClearOUT();
        }

        return;
    }
    if (USB_ControlRequest.bmRequestType == (REQDIR_HOSTTODEVICE | REQTYPE_CLASS | REQREC_INTERFACE)) {
        if (USB_ControlRequest.bRequest >= COMMAND_REBOOT && USB_ControlRequest.bRequest <= COMMAND_JUMP_BOOTLOADER_UNO) {
            Endpoint_ClearSETUP();
            Endpoint_ClearStatusStage();
            bootloaderState = (JUMP | USB_ControlRequest.bRequest);
            reboot();
        }
    }
    packet_header_t packet;
    packet.magic = VALID_PACKET;
    packet.id = CONTROL_REQUEST_ID;
    packet.len = sizeof(control_request_t);
    if ((USB_ControlRequest.bmRequestType & REQDIR_DEVICETOHOST) == REQDIR_HOSTTODEVICE) {
        packet.len += USB_ControlRequest.wLength;
    }
    writeData(&packet, sizeof(packet_header_t));
    writeData(&USB_ControlRequest, sizeof(USB_ControlRequest));
    if ((USB_ControlRequest.bmRequestType & REQDIR_DEVICETOHOST) == REQDIR_HOSTTODEVICE) {
        Endpoint_ClearSETUP();
        uint8_t len = USB_ControlRequest.wLength;
        while (len--) {
            uint8_t byte = Endpoint_Read_8();
            writeData(&byte, 1);
        }
        Endpoint_ClearStatusStage();
    } else {
        readControlData();
    }
}
void EVENT_USB_Device_ConfigurationChanged(void) {
    if (!serial) {
        packet_header_t packet = {
            VALID_PACKET, DEVICE_ID, sizeof(packet_header_t)};
        writeData(&packet, sizeof(packet_header_t));
        readControlData();
        return;
    }

    Endpoint_ConfigureEndpoint(DEVICE_EPADDR_IN, EP_TYPE_BULK, SERIAL_TX_SIZE, 1);
    Endpoint_ConfigureEndpoint(DEVICE_EPADDR_OUT, EP_TYPE_BULK, SERIAL_RX_SIZE, 1);
    Endpoint_ConfigureEndpoint(CDC_NOTIFICATION, EP_TYPE_INTERRUPT, SERIAL_NOTIFICATION_SIZE, 1);
}
uint16_t CALLBACK_USB_GetDescriptor(const uint16_t wValue,
                                    const uint16_t wIndex,
                                    const void** const descriptorAddress) {
    if (serial) {
        const uint8_t DescriptorType = (wValue >> 8);
        const void* Address = NULL;
        uint16_t Size = 0;

        switch (DescriptorType) {
            case DTYPE_Device:
                Address = &DeviceDescriptor;
                Size = sizeof(USB_Descriptor_Device_t);
                break;
            case DTYPE_Configuration:
                Address = &ConfigurationDescriptor;
                Size = sizeof(USB_Descriptor_Configuration_t);
                break;
        }
        Endpoint_ClearSETUP();
        Endpoint_Write_Control_PStream_LE(Address, Size);
        Endpoint_ClearOUT();
        return 0;
    }
    // pass request to 328p / mega
    descriptor_request_t packet = {
        header : {VALID_PACKET, DESCRIPTOR_ID, sizeof(descriptor_request_t)},
        wValue : wValue,
        wIndex : wIndex,
        wLength : USB_ControlRequest.wLength
    };
    writeData(&packet, sizeof(descriptor_request_t));
    readControlData();
    Endpoint_ClearOUT();
    return 0;
}

/** ISR to manage the reception of data from the serial port, placing received
 * bytes into a circular buffer for later transmission to the host.
 */
ISR(USART1_RX_vect, ISR_NAKED) {
    // This ISR doesnt change SREG. Whoa.
    asm volatile(
        "lds r3, %[UDR1_Reg]\n\t"       // (1) Load new Serial byte (UDR1) into r3
        "movw r4, r30\n\t"              // (1) Backup Z pointer (r30 -> r4, r31 -> r5)
        "in r30, %[writePointer]\n\t"   // (1) Load USARTtoUSB write buffer 8 bit
                                        // pointer to lower Z pointer
        "ldi r31, 0x01\n\t"             // (1) Set higher Z pointer to 0x01
        "st Z+, r3\n\t"                 // (2) Save UDR1 in Z pointer (USARTtoUSB write buffer)
                                        // and increment
        "out %[writePointer], r30\n\t"  // (1) Save back new USARTtoUSB buffer
                                        // pointer location
        "movw r30, r4\n\t"              // (1) Restore backuped Z pointer
        "reti\n\t"                      // (4) Exit ISR

        // Inputs:
        ::[UDR1_Reg] "m"(UDR1),  // Memory location of UDR1
        [writePointer] "I"(_SFR_IO_ADDR(
            USARTtoUSB_WritePtr))  // 8 bit pointer to USARTtoUSB write buffer
    );
}

ISR(USART1_UDRE_vect, ISR_NAKED) {
    // Another SREG-less ISR.
    asm volatile(
        "movw r4, r30\n\t"             // (1) Backup Z pointer (r30 -> r4, r31 -> r5)
        "in r30, %[readPointer]\n\t"   // (1) Load USBtoUSART read buffer 8 bit
                                       // pointer to lower Z pointer
        "ldi r31, 0x02\n\t"            // (1) Set higher Z pointer to 0x02
        "ld r3, Z+\n\t"                // (2) Load next byte from USBtoUSART buffer into r3
        "sts %[UDR1_Reg], r3\n\t"      // (2) Save r3 (next byte) in UDR1
        "out %[readPointer], r30\n\t"  // (1) Save back new USBtoUSART read
                                       // buffer pointer location
        "cbi %[readPointer], 7\n\t"    // (2) Wrap around for 128 bytes
        //     smart after-the-fact andi 0x7F without using SREG
        "movw r30, r4\n\t"             // (1) Restore backuped Z pointer
        "in r2, %[readPointer]\n\t"    // (1) Load USBtoUSART read buffer 8 bit
                                       // pointer to r2
        "lds r3, %[writePointer]\n\t"  // (1) Load USBtoUSART write buffer to r3
        "cpse r2, r3\n\t"              // (1/2) Check if USBtoUSART read buffer == USBtoUSART
                                       // write buffer
        "reti\n\t"                     // (4) They are not equal, more bytes coming soon!
        "ldi r30, 0x98\n\t"            // (1) Set r30 temporary to new UCSR1B setting
                                       // ((1<<RXCIE1) | (1 << RXEN1) | (1 << TXEN1))
        //     ldi needs an upper register, restore Z once more afterwards
        "sts %[UCSR1B_Reg], r30\n\t"  // (2) Turn off this interrupt (UDRIE1),
                                      // all bytes sent
        "movw r30, r4\n\t"            // (1) Restore backuped Z pointer again (was
                                      // overwritten again above)
        "reti\n\t"                    // (4) Exit ISR

        // Inputs:
        ::[UDR1_Reg] "m"(UDR1),  // Memory location of UDR1
        [readPointer] "I"(_SFR_IO_ADDR(
            USBtoUSART_ReadPtr)),  // 7 bit pointer to USBtoUSART read buffer
        [writePointer] "m"(
            USBtoUSART_WritePtr),  // 7 bit pointer to USBtoUSART write buffer
        [UCSR1B_Reg] "m"(UCSR1B)   // Memory location of UCSR1B
    );
}