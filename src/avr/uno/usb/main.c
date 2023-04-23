// This include needs to be at the top
#include "LUFAConfig.h"
// And then all others after it
#include <LUFA/Drivers/Board/Board.h>
#include <LUFA/Drivers/Board/LEDs.h>
#include <LUFA/Drivers/Peripheral/Serial.h>
#include <LUFA/Drivers/USB/Class/CDCClass.h>
#include <LUFA/Drivers/USB/USB.h>
#include <avr/boot.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/power.h>
#include <avr/wdt.h>

#include "commands.h"
#include "defines.h"
#include "endpoints.h"
#include "packets.h"
#include "progmem.h"
#include "serial_descriptors.h"
#include "serial_macros.h"
#define JUMP 0xDE00

/* USBtoUSART_WritePtr needs to be visible to ISR. */
/* USARTtoUSB_ReadPtr needs to be visible to CDC LineEncoding Event. */
volatile uint8_t USBtoUSART_WritePtr = 0;
volatile uint8_t USARTtoUSB_ReadPtr = 0;
static CDC_LineEncoding_t LineEncoding = {.BaudRateBPS = 0,
                                          .CharFormat = CDC_LINEENCODING_OneStopBit,
                                          .ParityType = CDC_PARITY_None,
                                          .DataBits = 8};

// if bootloaderState is set to JUMP, then the arduino will jump to bootloader
// mode after the next watchdog reset
volatile uint16_t bootloaderState __attribute__((section(".noinit")));
#define COMMAND_SKIP_WAIT 0x24
// Are we in usbserial mode or controller mode
bool serial = false;

// Handle receiving data from the 328p during a control request
int handleControlRequest() {
    register uint16_t tmp;
    INIT_TMP_SERIAL_TO_USB(tmp);
    uint8_t state = STATE_NO_PACKET;
    uint8_t type;
    uint8_t packetCount = 0;
    while (true) {
        WAIT_FOR_BYTES(tmp, 1);
        register uint8_t data;
        READ_BYTE_FROM_BUF(data, tmp);
        // First we need to read out the packet header
        if (state == STATE_NO_PACKET && data == VALID_PACKET) {
            state = STATE_READ_PACKET_TYPE;
        } else if (state == STATE_READ_PACKET_TYPE) {
            type = data;
            state = STATE_READ_LENGTH;
        } else if (state == STATE_READ_LENGTH) {
            if (type == CONTROL_REQUEST_VALIDATION_ID || type == DEVICE_ID) {
                state = STATE_READ_AND_RETURN;
                continue;
            }
            if (type == CONTROLLER_DATA_RESTART_USB_ID) {
                // The 328p  wants to restart the usb layer, so just reboot the microcontroller
                bootloaderState = (JUMP | COMMAND_SKIP_WAIT);
                reboot();
            }
            // For host to device requests, we don't have any more data to read as the host was writing the data
            if ((USB_ControlRequest.bmRequestType & REQDIR_DEVICETOHOST) == REQDIR_HOSTTODEVICE) {
                FINISH_READ(tmp);
                return 0;
            }
            packetCount = data;
            break;
        } else if (state == STATE_READ_AND_RETURN) {
            // We are done
            FINISH_READ(tmp);
            return data;
        }
    }
    // Header is read, acknowledge the setup packet, and start writing bytes out from the circular buffer.
    Endpoint_ClearSETUP();
    if (!packetCount) {
        Endpoint_ClearIN();
    }
    WAIT_FOR_BYTES(tmp, packetCount);
    bool lastPacketFull = false;
    while (packetCount || lastPacketFull) {
        if (Endpoint_IsOUTReceived())
            break;
        if (Endpoint_IsSETUPReceived())
            break;

        if (Endpoint_IsINReady()) {
            uint16_t bytesInEndpoint = Endpoint_BytesInEndpoint();

            while (packetCount && (bytesInEndpoint < USB_Device_ControlEndpointSize)) {
                register uint8_t data;
                READ_BYTE_FROM_BUF(data, tmp);
                Endpoint_Write_8(data);
                packetCount--;
                bytesInEndpoint++;
            }

            lastPacketFull = (bytesInEndpoint == USB_Device_ControlEndpointSize);
            Endpoint_ClearIN();
        }
    }
    Endpoint_ClearStatusStage();
    // If the packet ends early, we still need to finish reading everything out of the buffer
    while (packetCount--) {
        register uint8_t data;
        READ_BYTE_FROM_BUF(data, tmp);
    }
    FINISH_READ(tmp);
    return 0;
}

// Handle writing data for the controller endpoints (both in and out)
void handleControllerData() {
    uint8_t txcount;
    uint8_t type;
    register uint16_t tmp;
    uint8_t state = STATE_NO_PACKET;
    INIT_TMP_SERIAL_TO_USB(tmp);
    // Read the header
    while (true) {
        WAIT_FOR_BYTES(tmp, 1);
        register uint8_t data;
        READ_BYTE_FROM_BUF(data, tmp);
        if (state == STATE_NO_PACKET && data == VALID_PACKET) {
            state = STATE_READ_PACKET_TYPE;
        } else if (state == STATE_READ_PACKET_TYPE) {
            type = data;
            state = STATE_READ_LENGTH;
        } else if (state == STATE_READ_LENGTH) {
            txcount = data;
            // If the packet was host to device, than we are done here as the data was written already
            if (type == CONTROLLER_DATA_TRANSMIT_ID) {
                FINISH_READ(tmp);
                return;
            }
            break;
        }
    }
    // This is device to host, so write out all the data to the endpoint
    WAIT_FOR_BYTES(tmp, txcount);
    while (txcount--) {
        register uint8_t data;
        READ_BYTE_FROM_BUF(data, tmp);
        Endpoint_Write_8(data);
    }
    FINISH_READ(tmp);
}
int main(void) {
    bool skip_wait = false;
    // Handle jumping to different states depending on bootloaderState (which is preserved on a reboot)
    if (bootloaderState == (JUMP | COMMAND_JUMP_BOOTLOADER)) {
        // We don't want to jump again after the bootloader returns control flow to
        // us
        bootloaderState = 0;
        asm volatile("jmp 0x1000");
    }
    if (bootloaderState == (JUMP | COMMAND_JUMP_BOOTLOADER_UNO_USB_THEN_SERIAL)) {
        // When the bootloader returns control flow to us, we then want to jump to serial mode
        bootloaderState = (JUMP | COMMAND_JUMP_BOOTLOADER_UNO);
        asm volatile("jmp 0x1000");
    } else if (bootloaderState == (JUMP | COMMAND_JUMP_BOOTLOADER_UNO)) {
        serial = true;
    } else if (bootloaderState == (JUMP | COMMAND_SKIP_WAIT)) {
        skip_wait = true;
    }
    // We don't want to jump again after the bootloader returns control flow to
    // us
    bootloaderState = 0;

    /* Disable watchdog if enabled by bootloader/fuses */
    MCUSR &= ~(1 << WDRF);
    wdt_disable();

    clock_prescale_set(clock_div_1);

    // Configure the serial port for controller mode, usbserial mode will reconfigure it later
    UCSR1C = ((1 << UCSZ11) | (1 << UCSZ10));
    UCSR1A = (1 << U2X1);
    UCSR1B = ((1 << TXEN1) | (1 << RXEN1));
    UBRR1 = SERIAL_2X_UBBRVAL(BAUD);

    if (skip_wait) {
        // Soft reset, assume the microcontroller is already booted and just continue
        AVR_RESET_LINE_PORT |= AVR_RESET_LINE_MASK;
        sei();
    } else {
        // Make sure that the 328p is fully reset, as it sends us the ready byte on bootup
        AVR_RESET_LINE_DDR |= AVR_RESET_LINE_MASK;
        AVR_RESET_LINE_PORT |= AVR_RESET_LINE_MASK;
        _delay_ms(1);
        AVR_RESET_LINE_PORT &= ~AVR_RESET_LINE_MASK;
        _delay_ms(1);
        AVR_RESET_LINE_PORT |= AVR_RESET_LINE_MASK;

        sei();
        if (serial) {
            // Enable a timer to rapidly flush bytes over serial
            TCCR0B = (1 << CS02);
        } else {
            // Wait for the 328p / 1280 to boot (we receive a READY byte)
            // If we don't get it in time, we can assume the 328p is missing its firmware or has a wonky config and drop to the 328p programming mode
            uint16_t count = 0;
            uint8_t count2 = 0;
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
    }
    // Now that we have the ready byte above, we can turn on the serial receive interrupt handler
    UCSR1B = ((1 << TXEN1) | (1 << RXCIE1) | (1 << RXEN1));

    USB_Init();

    // Loop for handling serial data
    if (serial) {
        while (true) {
            USB_USBTask();
            // Send any data from the serial out buffer over USB
            Endpoint_SelectEndpoint(DEVICE_EPADDR_IN);
            if (Endpoint_IsINReady()) {
                uint16_t tmp;
                INIT_TMP_SERIAL_TO_USB(tmp);
                uint8_t txcount;
                uint8_t count = USARTtoUSB_WritePtr - USARTtoUSB_ReadPtr;
                // Check if the UART receive buffer flush timer has expired or the buffer is nearly full
                if (!((TIFR0 & (1 << TOV0)) || (count >= (SERIAL_TX_SIZE - 1)))) {
                    continue;
                }
                TIFR0 = (1 << TOV0);
                txcount = SERIAL_TX_SIZE - 1;
                if (txcount > count) {
                    txcount = count;
                }
                while (txcount--) {
                    register uint8_t data;
                    READ_BYTE_FROM_BUF(data, tmp);
                    Endpoint_Write_8(data);
                }
                // Save new pointer position
                FINISH_READ(tmp);
                Endpoint_ClearIN();
            }

            // Send any data received from usb to the serial in buffer
            Endpoint_SelectEndpoint(DEVICE_EPADDR_OUT);
            uint8_t countRX = 0;
            if (Endpoint_IsOUTReceived()) {
                // Check if we received any new bytes
                countRX = Endpoint_BytesInEndpoint();

                // Acknowledge zero length packet
                if (!countRX)
                    Endpoint_ClearOUT();
            }
            // Check if there is space for the data to receive
            uint8_t USBtoUSART_free = (USB2USART_BUFLEN - 1) - ((USBtoUSART_WritePtr - USBtoUSART_ReadPtr) & (USB2USART_BUFLEN - 1));
            if (countRX && countRX <= USBtoUSART_free) {
                // There is space, receive the data
                uint16_t tmp;
                INIT_TMP_USB_TO_SERIAL(tmp);
                while (countRX--) {
                    register uint8_t data = Endpoint_Read_8();
                    WRITE_BYTE_TO_BUF(data, tmp);
                }
                // Acknowledge the read
                Endpoint_ClearOUT();
                FINISH_WRITE(tmp);
            }
        }
    }

    // Controller mode, construct packets that we will need to send later
    packet_header_t requestData = {
        VALID_PACKET, CONTROLLER_DATA_REQUEST_ID, sizeof(packet_header_t)};
    data_transmit_packet_t packet = {
        {VALID_PACKET, CONTROLLER_DATA_TRANSMIT_ID, 0}};

    // Loop for handling controller mode
    while (true) {
        USB_USBTask();

        // Check if the host is ready to receive a controller packet
        Endpoint_SelectEndpoint(DEVICE_EPADDR_IN);
        if (Endpoint_IsINReady()) {
            // It is, request one from the 328p and then send it
            uint16_t tmp;
            INIT_TMP_USB_TO_SERIAL(tmp);
            WRITE_ARRAY_TO_BUF(tmp, &requestData, sizeof(packet_header_t));

            // Now wait for and process the response from the 328p
            FINISH_WRITE(tmp);
            handleControllerData();
            // Acknowledge the packet
            Endpoint_ClearIN();
            continue;
        }

        // Check if the host has sent us a controller packet (for example, LEDs or other controller info)
        Endpoint_SelectEndpoint(DEVICE_EPADDR_OUT);
        if (Endpoint_IsOUTReceived()) {
            // It has, send the packet to the 328p for processing
            uint16_t tmp;
            INIT_TMP_USB_TO_SERIAL(tmp);
            packet.header.len = sizeof(data_transmit_packet_t) + Endpoint_BytesInEndpoint();
            WRITE_ARRAY_TO_BUF(tmp, &packet, sizeof(data_transmit_packet_t));
            // Check if the packet is empty or has data
            if (Endpoint_IsReadWriteAllowed()) {
                uint8_t count = Endpoint_BytesInEndpoint();
                while (count--) {
                    register uint8_t data = Endpoint_Read_8();
                    WRITE_BYTE_TO_BUF(data, tmp);
                }
            }

            // Now wait for and process the response from the 328p
            FINISH_WRITE(tmp);
            handleControllerData();
            // Acknowledge the packet
            Endpoint_ClearOUT();
        }
    }
}

void write_ready(bool ready) {
    uint16_t tmp;
    INIT_TMP_USB_TO_SERIAL(tmp);
    packet_header_t control_request_packet = {VALID_PACKET, USB_READY, sizeof(usb_ready_t)};
    WRITE_ARRAY_TO_BUF(tmp, &control_request_packet, sizeof(packet_header_t));
    WRITE_ARRAY_TO_BUF(tmp, &ready, 1);
    FINISH_WRITE(tmp);
}

void EVENT_USB_Device_Connect(void) {
    write_ready(true);
}

void EVENT_USB_Device_Disconnect(void) {
    write_ready(false);
}

void EVENT_USB_Device_ControlRequest(void) {
    if (!(Endpoint_IsSETUPReceived())) return;
    // Standard requests NEED to shortcut this system!
    if ((USB_ControlRequest.bmRequestType & (REQTYPE_VENDOR | REQTYPE_CLASS)) == REQTYPE_STANDARD) {
        return;
    }
    // Handle usbserial related control requests
    if (serial) {
        if (USB_ControlRequest.bmRequestType == (REQDIR_HOSTTODEVICE | REQTYPE_CLASS | REQREC_INTERFACE)) {
            if (USB_ControlRequest.bRequest == CDC_REQ_SetLineEncoding) {
                Endpoint_ClearSETUP();

                /* Read the line coding data in from the host into the global struct */
                Endpoint_Read_Control_Stream_LE(&LineEncoding, sizeof(CDC_LineEncoding_t));
                Endpoint_ClearIN();

                // Handle the "1200bps touch" that pro micros use for jumping to the bootloader, only we reuse it here to jump to dfu moode
                if (LineEncoding.BaudRateBPS == 1200) {
                    bootloaderState = (JUMP | COMMAND_JUMP_BOOTLOADER);
                    reboot();
                } else if (LineEncoding.BaudRateBPS == 2400) {
                    // And we have our own "2400bps touch" that just reboots, which will jump from usbserial back to controller mode
                    reboot();
                }

                /* Keep the TX line held high (idle) while the USART is reconfigured */
                PORTD |= (1 << 3);

                /* Flush data that was about to be sent. */
                USBtoUSART_ReadPtr = 0;
                USBtoUSART_WritePtr = 0;
                USARTtoUSB_ReadPtr = 0;
                USARTtoUSB_WritePtr = 0;

                // Reconfigure the USART based on the LineEncoding
                int8_t ConfigMask = 0;

                switch (LineEncoding.ParityType) {
                    case CDC_PARITY_Odd:
                        ConfigMask = ((1 << UPM11) | (1 << UPM10));
                        break;
                    case CDC_PARITY_Even:
                        ConfigMask = (1 << UPM11);
                        break;
                }

                if (LineEncoding.CharFormat == CDC_LINEENCODING_TwoStopBits)
                    ConfigMask |= (1 << USBS1);

                switch (LineEncoding.DataBits) {
                    case 6:
                        ConfigMask |= (1 << UCSZ10);
                        break;
                    case 7:
                        ConfigMask |= (1 << UCSZ11);
                        break;
                    case 8:
                        ConfigMask |= ((1 << UCSZ11) | (1 << UCSZ10));
                        break;
                }

                // Set the new baud rate before configuring the USART
                uint8_t clockSpeed = (1 << U2X1);
                uint16_t brr = SERIAL_2X_UBBRVAL(LineEncoding.BaudRateBPS);

                // No need U2X or cant have U2X.
                if ((brr & 1) || (brr > 4095)) {
                    brr >>= 1;
                    clockSpeed = 0;
                }

                // Or special case 57600 baud for compatibility with the ATmega328 bootloader.
                else if (brr == SERIAL_2X_UBBRVAL(57600)) {
                    brr = SERIAL_UBBRVAL(57600);
                    clockSpeed = 0;
                }

                UBRR1 = brr;

                // Reconfigure the USART
                UCSR1C = ConfigMask;
                UCSR1A = clockSpeed;
                UCSR1B = ((1 << RXCIE1) | (1 << TXEN1) | (1 << RXEN1));

                /* Release the TX line after the USART has been reconfigured */
                PORTD &= ~(1 << 3);
            } else if (USB_ControlRequest.bRequest == CDC_REQ_SetControlLineState) {
                Endpoint_ClearSETUP();
                Endpoint_ClearStatusStage();

                // Handle the DTR line resetting the UNO, as the arduino uno uses this for programming
                if (USB_ControlRequest.wValue & CDC_CONTROL_LINE_OUT_DTR) {
                    AVR_RESET_LINE_PORT &= ~AVR_RESET_LINE_MASK;
                } else {
                    AVR_RESET_LINE_PORT |= AVR_RESET_LINE_MASK;
                }
            }
        } else if (USB_ControlRequest.bmRequestType == (REQDIR_DEVICETOHOST | REQTYPE_CLASS | REQREC_INTERFACE) && USB_ControlRequest.bRequest == CDC_REQ_GetLineEncoding) {
            // If the host requests the current line encoding return it
            Endpoint_ClearSETUP();
            Endpoint_Write_Control_Stream_LE(&LineEncoding, sizeof(CDC_LineEncoding_t));
            Endpoint_ClearOUT();
        }

        return;
    }
    // Handle control requests for controller mode
    // We uses a few control requests to jump between different modes on the uno, so handle that here
    if (USB_ControlRequest.bmRequestType == (REQDIR_HOSTTODEVICE | REQTYPE_CLASS | REQREC_INTERFACE)) {
        if (USB_ControlRequest.bRequest >= COMMAND_REBOOT && USB_ControlRequest.bRequest <= COMMAND_JUMP_BOOTLOADER_UNO_USB_THEN_SERIAL) {
            Endpoint_ClearSETUP();
            Endpoint_ClearStatusStage();
            // We cheat a little and use the commands for jumping as flags for when to jump to different modes
            bootloaderState = (JUMP | USB_ControlRequest.bRequest);
            reboot();
        }
    }

    // Ask the 328p if wants to handle a control request, by sending it a validation header and the control request header
    uint16_t tmp;
    INIT_TMP_USB_TO_SERIAL(tmp);
    packet_header_t control_request_packet = {VALID_PACKET, CONTROL_REQUEST_VALIDATION_ID, sizeof(control_request_t)};
    WRITE_ARRAY_TO_BUF(tmp, &control_request_packet, sizeof(packet_header_t));
    WRITE_ARRAY_TO_BUF(tmp, &USB_ControlRequest, sizeof(USB_ControlRequest));
    FINISH_WRITE(tmp);
    if (!handleControlRequest()) {
        // It does not want to handle the control request, return
        return;
    }

    // Handle the control request
    control_request_packet.id = CONTROL_REQUEST_ID;

    // For host to device requests, we also send the data itself so increase the length
    if ((USB_ControlRequest.bmRequestType & REQDIR_DEVICETOHOST) == REQDIR_HOSTTODEVICE) {
        control_request_packet.len += USB_ControlRequest.wLength;
    }
    WRITE_ARRAY_TO_BUF(tmp, &control_request_packet, sizeof(packet_header_t));
    WRITE_ARRAY_TO_BUF(tmp, &USB_ControlRequest, sizeof(USB_ControlRequest));
    // For host to device requests, send the data from the host
    if ((USB_ControlRequest.bmRequestType & REQDIR_DEVICETOHOST) == REQDIR_HOSTTODEVICE) {
        Endpoint_ClearSETUP();
        uint8_t len = USB_ControlRequest.wLength;
        while (len) {
            register uint8_t data = Endpoint_Read_8();
            WRITE_BYTE_TO_BUF(data, tmp);
            len--;
        }
        Endpoint_ClearStatusStage();
    }

    // Now wait for and process the response from the 328p.
    FINISH_WRITE(tmp);
    handleControlRequest();
}
void EVENT_USB_Device_ConfigurationChanged(void) {
    if (serial) {
        // Configure endpoints for usbserial mode
        Endpoint_ConfigureEndpoint(DEVICE_EPADDR_IN, EP_TYPE_BULK, SERIAL_TX_SIZE, 1);
        Endpoint_ConfigureEndpoint(DEVICE_EPADDR_OUT, EP_TYPE_BULK, SERIAL_RX_SIZE, 1);
        Endpoint_ConfigureEndpoint(CDC_NOTIFICATION, EP_TYPE_INTERRUPT, SERIAL_NOTIFICATION_SIZE, 1);
        return;
    }
    // When in controller mode, we need to ask the 328p what type of controller we are emulating, so we can configure the endpoints correctly
    uint16_t tmp;
    INIT_TMP_USB_TO_SERIAL(tmp);
    packet_header_t packet = {
        VALID_PACKET, DEVICE_ID, sizeof(packet_header_t)};
    WRITE_ARRAY_TO_BUF(tmp, &packet, sizeof(packet_header_t));

    // Now wait for and process the response from the 328p
    FINISH_WRITE(tmp);
    uint8_t consoleType = handleControlRequest();

    // Now that we have the console type, we can configure the endpoints accordingly
    uint8_t type = EP_TYPE_INTERRUPT;
    uint8_t epsize = 0x20;
    if (consoleType == WINDOWS || consoleType == XBOX360) {
        epsize = 0x18;
    }
    if (consoleType == MIDI) {
        type = EP_TYPE_BULK;
    }
    Endpoint_ConfigureEndpoint(DEVICE_EPADDR_IN, type, epsize, 2);
    Endpoint_ConfigureEndpoint(DEVICE_EPADDR_OUT, type, 0x08, 2);
    return;
}

uint16_t CALLBACK_USB_GetDescriptor(const uint16_t wValue,
                                    const uint16_t wIndex,
                                    const void** const descriptorAddress) {
    if (serial) {
        // For usbserial mode, we have descriptors from "serial_descriptors.h" that we send
        const uint8_t descriptorType = (wValue >> 8);
        const void* address = NULL;
        uint16_t size = 0;

        switch (descriptorType) {
            case DTYPE_Device:
                address = &usbSerialDeviceDescriptor;
                size = sizeof(USB_Descriptor_Device_t);
                break;
            case DTYPE_Configuration:
                address = &usbSerialConfigurationDescriptor;
                size = sizeof(USB_Descriptor_Configuration_t);
                break;
        }
        if (address) {
            // To avoid needing to enable support for different descriptor areas
            // we can write the descriptor outselves and act like we don't have a descriptor to write
            Endpoint_ClearSETUP();
            Endpoint_Write_Control_PStream_LE(address, size);
            Endpoint_ClearOUT();
        }
        return 0;
    }
    // pass request to 328p / mega
    descriptor_request_t packet = {
        header : {VALID_PACKET, DESCRIPTOR_ID, sizeof(descriptor_request_t)},
        wValue : wValue,
        wIndex : wIndex,
        wLength : USB_ControlRequest.wLength
    };
    uint16_t tmp;
    INIT_TMP_USB_TO_SERIAL(tmp);
    WRITE_ARRAY_TO_BUF(tmp, &packet, sizeof(descriptor_request_t));

    // wait for a response from the 328p. At this point we can handle it exactly the same was as a control request
    FINISH_WRITE(tmp);
    handleControlRequest();
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