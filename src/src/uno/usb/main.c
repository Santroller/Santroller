// This include needs to be at the top
#include "circular_buffer.h"
// And then all others after it
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
static CDC_LineEncoding_t LineEncoding = {.BaudRateBPS = 0,
                                          .CharFormat = CDC_LINEENCODING_OneStopBit,
                                          .ParityType = CDC_PARITY_None,
                                          .DataBits = 8};

// if bootloaderState is set to JUMP, then the arduino will jump to bootloader
// mode after the next watchdog reset
volatile uint16_t bootloaderState __attribute__((section(".noinit")));

volatile bool ready = false;
bool serial = false;

bool waitingForData = true;

void readControlOutData() {
    uint16_t tmp;
    INIT_TMP_BUF(tmp);
    uint8_t state = STATE_NO_PACKET;
    uint8_t packetCount = 0;
    while (true) {
        // Wait for a byte
        while (USARTtoUSB_WritePtr - (tmp & 0xff) == 0) {
        }

        register uint8_t data;
        READ_BYTE_FROM_BUF(data, tmp);
        if (state == STATE_NO_PACKET && data == VALID_PACKET) {
            state = STATE_READ_PACKET_TYPE;
        } else if (state == STATE_READ_PACKET_TYPE) {
            if (data == CONTROL_REQUEST_ID) {
                state = STATE_READ_LENGTH;
            } else if (data == CONTROL_REQUEST_INVALID_ID) {
                state = STATE_READ_INVALID_CONTROL_REQUEST_LENGTH;
            }
        } else if (state == STATE_READ_INVALID_CONTROL_REQUEST_LENGTH) {
            USARTtoUSB_ReadPtr = tmp & 0xFF;
            return;
        } else if (state == STATE_READ_LENGTH) {
            packetCount = data;
            break;
        } 
    }
    
    Endpoint_ClearSETUP();
    Endpoint_Write_Control_Buffer_LE(&tmp, packetCount);
    // With a control request, we can end early. Due to this we can't rely on tmp having read all the data, so we just reset both pointers
    USARTtoUSB_ReadPtr = 0;
    USARTtoUSB_WritePtr = 0;
    Endpoint_ClearOUT();
}

void readControlData() {
    uint16_t tmp;
    INIT_TMP_BUF(tmp);
    uint8_t state = STATE_NO_PACKET;
    uint8_t packetCount = 0;
    while (true) {
        // Wait for a byte
        while (USARTtoUSB_WritePtr - (tmp & 0xff) == 0) {
        }

        register uint8_t data;
        READ_BYTE_FROM_BUF(data, tmp);
        if (state == STATE_NO_PACKET && data == VALID_PACKET) {
            state = STATE_READ_PACKET_TYPE;
        } else if (state == STATE_READ_PACKET_TYPE) {
            if (data == DEVICE_ID) {
                state = STATE_READ_DEVICE_ID;
            } else if (data == DESCRIPTOR_ID || data == CONTROL_REQUEST_ID) {
                state = STATE_READ_LENGTH;
            } 
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
    Endpoint_ClearSETUP();
    // Now wait to read the whole packet
    while (USARTtoUSB_WritePtr - (tmp & 0xff) < packetCount) {
    }
    Endpoint_Write_Control_Buffer_LE(&tmp, packetCount);
    // With a control request, we can end early. Due to this we can't rely on tmp having read all the data, so we just reset both pointers
    USARTtoUSB_ReadPtr = 0;
    USARTtoUSB_WritePtr = 0;
    Endpoint_ClearOUT();
}
void readSerialData() {
    uint16_t tmp;
    INIT_TMP_BUF(tmp);
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
    uint16_t tmp;
    INIT_TMP_BUF(tmp);
    uint8_t txcount;
    uint8_t state = STATE_NO_PACKET;
    while (true) {
        // Wait for a byte
        while (USARTtoUSB_WritePtr - (tmp & 0xff) == 0) {
        }

        register uint8_t data;
        READ_BYTE_FROM_BUF(data, tmp);
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

bool validControlRequest() {
    // Note, if something is added here, it also needs to be mirrored in usb_endpoints/src/commands.c
    if (USB_ControlRequest.bmRequestType == (REQDIR_DEVICETOHOST | REQREC_INTERFACE | REQTYPE_VENDOR)) {
        if (USB_ControlRequest.bRequest == HID_REQ_GetReport && USB_ControlRequest.wIndex == INTERFACE_ID_Device && USB_ControlRequest.wValue == 0x0000) {
            return true;
        } else if (USB_ControlRequest.bRequest == REQ_GET_OS_FEATURE_DESCRIPTOR && USB_ControlRequest.wIndex == DESC_EXTENDED_PROPERTIES_DESCRIPTOR && USB_ControlRequest.wValue == INTERFACE_ID_Config) {
            return true;
        } else if (USB_ControlRequest.bRequest == HID_REQ_GetReport && USB_ControlRequest.wIndex == INTERFACE_ID_Device && USB_ControlRequest.wValue == 0x0100) {
            return true;
        }
    } else if (USB_ControlRequest.bmRequestType == (REQDIR_DEVICETOHOST | REQREC_INTERFACE | REQTYPE_VENDOR)) {
        if (USB_ControlRequest.bRequest == REQ_GET_OS_FEATURE_DESCRIPTOR && USB_ControlRequest.wIndex == DESC_EXTENDED_COMPATIBLE_ID_DESCRIPTOR) {
            return true;
        } else if (USB_ControlRequest.bRequest == HID_REQ_GetReport && USB_ControlRequest.wIndex == 0x00 && USB_ControlRequest.wValue == 0x0000) {
            return true;
        }
    } else if (USB_ControlRequest.bmRequestType == (REQDIR_DEVICETOHOST | REQREC_INTERFACE | REQTYPE_CLASS) && USB_ControlRequest.wIndex == 0x0300) {
        return true;
    } else if (USB_ControlRequest.bRequest == HID_REQ_SetReport && USB_ControlRequest.bmRequestType == (REQDIR_HOSTTODEVICE | REQREC_INTERFACE | REQTYPE_CLASS)) {
    } else if (USB_ControlRequest.bRequest == REQ_ClearFeature && (USB_ControlRequest.wIndex == DEVICE_EPADDR_IN || USB_ControlRequest.wIndex == DEVICE_EPADDR_OUT)) {
        return true;
    }
    return false;
}

void EVENT_USB_Device_ControlRequest(void) {
    if (!(Endpoint_IsSETUPReceived())) return;
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
    if (!validControlRequest()) {
        return;
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
    return 0;
}
