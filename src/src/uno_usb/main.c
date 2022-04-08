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
#include <LUFA/LUFA/Drivers/Misc/RingBuffer.h>
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
#define JUMP 0xDE00
#define BUFFER_NEARLY_FULL 96
static CDC_LineEncoding_t LineEncoding = {.BaudRateBPS = 0,
                                          .CharFormat = CDC_LINEENCODING_OneStopBit,
                                          .ParityType = CDC_PARITY_None,
                                          .DataBits = 8};
typedef struct
{
    USB_Descriptor_Configuration_Header_t Config;
    USB_Descriptor_Interface_t CDC_CCI_Interface;
    USB_CDC_Descriptor_FunctionalHeader_t CDC_Functional_Header;
    USB_CDC_Descriptor_FunctionalACM_t CDC_Functional_ACM;
    USB_CDC_Descriptor_FunctionalUnion_t CDC_Functional_Union;
    USB_Descriptor_Endpoint_t CDC_NotificationEndpoint;
    USB_Descriptor_Interface_t CDC_DCI_Interface;
    USB_Descriptor_Endpoint_t CDC_DataOutEndpoint;
    USB_Descriptor_Endpoint_t CDC_DataInEndpoint;
} USB_Descriptor_Configuration_t;
const USB_Descriptor_Configuration_t PROGMEM ConfigurationDescriptor;
const USB_Descriptor_Device_t PROGMEM DeviceDescriptor;
uint8_t buf_desc[sizeof(ConfigurationDescriptor)];
// if bootloaderState is set to JUMP, then the arduino will jump to bootloader
// mode after the next watchdog reset
uint16_t bootloaderState __attribute__((section(".noinit")));

uint8_t* bufTx;
uint8_t idx_tx = 0;

uint8_t buf[128];
uint8_t bufout[128];
/** Circular buffer to hold data from the host before it is sent to the device via the serial port. */
RingBuffer_t USBtoUSART_Buffer;

/** Circular buffer to hold data from the serial port before it is sent to the host. */
RingBuffer_t USARTtoUSB_Buffer;

packet_header_t* header = (packet_header_t*)buf;
ret_packet_t* ret = (ret_packet_t*)buf;
volatile bool ready = false;
bool serial = false;

void txRX(void* data, uint8_t len) {
    idx_tx = len;
    bufTx = (uint8_t*)data;
    UCSR1B = (_BV(TXEN1) | _BV(RXEN1) | _BV(UDRIE1));
    while (!ready) {
    }
    ready = false;
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
    UCSR1B = ((1 << TXEN1) | (1 << RXCIE1) | (1 << RXEN1));
    UBRR1 = SERIAL_2X_UBBRVAL(BAUD);

    DDRD |= (1 << 3);
    PORTD |= (1 << 2);
    AVR_RESET_LINE_DDR |= AVR_RESET_LINE_MASK;
    AVR_RESET_LINE_PORT &= ~AVR_RESET_LINE_MASK;
    _delay_ms(1);
    AVR_RESET_LINE_PORT |= AVR_RESET_LINE_MASK;
    sei();
    if (serial) {
        USB_Init();
        RingBuffer_InitBuffer(&USBtoUSART_Buffer, buf, sizeof(buf));
        RingBuffer_InitBuffer(&USARTtoUSB_Buffer, bufout, sizeof(bufout));
        TCCR0B = (1 << CS02);
        while (true) {
            USB_USBTask();
            /* Only try to read in bytes from the CDC interface if the transmit buffer is not full */
            if (!(RingBuffer_IsFull(&USBtoUSART_Buffer))) {
                Endpoint_SelectEndpoint(DEVICE_EPADDR_OUT);
                if (Endpoint_IsOUTReceived()) {
                    if (Endpoint_BytesInEndpoint()) {
                        RingBuffer_Insert(&USBtoUSART_Buffer, Endpoint_Read_8());
                    }
                    if (!(Endpoint_BytesInEndpoint())) {
                        Endpoint_ClearOUT();
                    }
                }
            }

            Endpoint_SelectEndpoint(DEVICE_EPADDR_IN);
            /* Check if the UART receive buffer flush timer has expired or the buffer is nearly full */
            uint16_t BufferCount = RingBuffer_GetCount(&USARTtoUSB_Buffer);
            if ((TIFR0 & (1 << TOV0)) || (BufferCount > BUFFER_NEARLY_FULL)) {
                TIFR0 |= (1 << TOV0);
                /* Read bytes from the USART receive buffer into the USB IN endpoint */
                while (BufferCount--) {
                    if (!(Endpoint_IsReadWriteAllowed())) {
                        Endpoint_ClearIN();
                        Endpoint_WaitUntilReady();
                    }
                    Endpoint_Write_8(RingBuffer_Remove(&USARTtoUSB_Buffer));
                }
            }
            if (Endpoint_BytesInEndpoint()) {
                bool BankFull = !(Endpoint_IsReadWriteAllowed());

                Endpoint_ClearIN();

                if (BankFull) {
                    Endpoint_WaitUntilReady();

                    Endpoint_ClearIN();
                }
            }

            /* Load the next byte from the USART transmit buffer into the USART */
            if (!(RingBuffer_IsEmpty(&USBtoUSART_Buffer))) {
                Serial_SendByte(RingBuffer_Remove(&USBtoUSART_Buffer));
            }
        }
    }
    // Wait for the 328p / 1280 to boot (we receive a READY byte)
    // Since it is set in the isr, we need to treat it as volatile
    while (((volatile uint8_t*)buf)[0] != READY) {
    }
    USB_Init();
    packet_header_t requestData = {
        VALID_PACKET, CONTROLLER_DATA_REQUEST_ID, sizeof(packet_header_t)};
    while (true) {
        USB_USBTask();
        Endpoint_SelectEndpoint(DEVICE_EPADDR_IN);
        if (Endpoint_IsINReady()) {
            txRX(&requestData, sizeof(packet_header_t));
            uint8_t len = ret->header.len - sizeof(packet_header_t);
            uint8_t* tmp = ret->data;
            Endpoint_Write_Stream_LE(tmp, len, NULL);
            Endpoint_ClearIN();
        }
        data_transmit_packet_t* packet = (data_transmit_packet_t*)buf;
        Endpoint_SelectEndpoint(DEVICE_EPADDR_OUT);
        if (Endpoint_IsOUTReceived()) {
            packet->header.magic = VALID_PACKET;
            packet->header.id = CONTROLLER_DATA_TRANSMIT_ID;
            packet->header.len = sizeof(control_request_t) + Endpoint_BytesInEndpoint();
            /* Check to see if the packet contains data */
            if (Endpoint_IsReadWriteAllowed()) {
                Endpoint_Read_Stream_LE(packet->data, Endpoint_BytesInEndpoint(), NULL);
                txRX(packet, packet->header.len);
            }
            Endpoint_ClearOUT();
        }
    }
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
                } else if (LineEncoding.BaudRateBPS == 2400) {
                    bootloaderState = JUMP;
                }
                if (bootloaderState) {
                    cli();
                    wdt_enable(WDTO_15MS);
                    for (;;) {
                    }
                }

                /* Must turn off USART before reconfiguring it, otherwise incorrect operation may occur */
                UCSR1B = 0;
                UCSR1A = 0;
                UCSR1C = 0;

                UBRR1 = SERIAL_2X_UBBRVAL(LineEncoding.BaudRateBPS);

                UCSR1C = ((1 << UCSZ11) | (1 << UCSZ10));
                UCSR1A = (1 << U2X1);
                UCSR1B = ((1 << RXCIE1) | (1 << TXEN1) | (1 << RXEN1));
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
            cli();
            wdt_enable(WDTO_15MS);
            for (;;) {
            }
            return;
        }
    }
    control_request_t* packet = (control_request_t*)buf;
    packet->header.magic = VALID_PACKET;
    packet->header.id = CONTROL_REQUEST_ID;
    packet->header.len = sizeof(control_request_t);
    packet->bmRequestType = USB_ControlRequest.bmRequestType;
    packet->request = USB_ControlRequest.bRequest;
    packet->wValue = USB_ControlRequest.wValue;
    packet->wIndex = USB_ControlRequest.wIndex;
    packet->wLength = USB_ControlRequest.wLength;
    if (packet->bmRequestType == (REQDIR_HOSTTODEVICE | REQTYPE_VENDOR | REQREC_INTERFACE)) {
        Endpoint_ClearSETUP();
        packet->header.len += USB_ControlRequest.wLength;
        Endpoint_Read_Control_Stream_LE(packet->data, USB_ControlRequest.wLength);
        Endpoint_ClearStatusStage();
    }
    txRX(packet, packet->header.len);
    if (ret->data[0] && packet->bmRequestType == (REQDIR_DEVICETOHOST | REQTYPE_VENDOR | REQREC_INTERFACE)) {
        Endpoint_ClearSETUP();
        Endpoint_Write_Control_Stream_LE(ret->data + 1, ret->header.len - sizeof(packet_header_t) - 1);
        Endpoint_ClearStatusStage();
    }
}
void EVENT_USB_Device_ConfigurationChanged(void) {
    if (!serial) {
        uint8_t type = EP_TYPE_INTERRUPT;
        uint8_t epsize = 0x20;
        packet_header_t packet = {
            VALID_PACKET, DEVICE_ID, sizeof(packet_header_t)};
        txRX(&packet, sizeof(packet_header_t));
        uint8_t consoleType = ret->data[1];
        if (consoleType == XBOX360 || consoleType == PC_XINPUT) {
            epsize = 0x18;
        }
        if (consoleType == MIDI) {
            type = EP_TYPE_BULK;
        }
        Endpoint_ConfigureEndpoint(DEVICE_EPADDR_IN, type, epsize, 1);
        Endpoint_ConfigureEndpoint(DEVICE_EPADDR_OUT, type, 0x08, 2);
        return;
    }

    Endpoint_ConfigureEndpoint(DEVICE_EPADDR_IN, EP_TYPE_BULK, SERIAL_ENDPOINT_SIZE, 1);
    Endpoint_ConfigureEndpoint(DEVICE_EPADDR_OUT, EP_TYPE_BULK, SERIAL_ENDPOINT_SIZE, 1);
    Endpoint_ConfigureEndpoint(CDC_NOTIFICATION, EP_TYPE_INTERRUPT, SERIAL_ENDPOINT_SIZE, 1);
}

uint16_t CALLBACK_USB_GetDescriptor(const uint16_t wValue,
                                    const uint16_t wIndex,
                                    const void** const descriptorAddress) {
    if (serial) {
        const uint8_t DescriptorType = (wValue >> 8);
        const void* Address = NULL;
        uint16_t Size = NO_DESCRIPTOR;

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
        memcpy_P(buf_desc, Address, Size);
        *descriptorAddress = buf_desc;
        return Size;
    }
    // pass request to 328p / mega
    descriptor_request_t packet = {
        header : DESC_REQUEST_HEADER,
        wValue : wValue,
        wIndex : wIndex
    };
    txRX(&packet, sizeof(descriptor_request_t));
    *descriptorAddress = ret->data;
    return ret->header.len - sizeof(packet_header_t);
}

uint8_t idx = 0;
ISR(USART1_RX_vect, ISR_BLOCK) {
    if (serial) {
        RingBuffer_Insert(&USARTtoUSB_Buffer, UDR1);
        return;
    }
    buf[idx] = UDR1;
    if (idx == 0 && buf[0] != VALID_PACKET) {
        return;
    }
    idx++;

    if (idx >= sizeof(packet_header_t) && header->len == idx) {
        idx = 0;
        ready = true;
        return;
    }
}

ISR(USART1_UDRE_vect, ISR_BLOCK) {
    if (idx_tx--) {
        UDR1 = *(bufTx++);
    } else {
        UCSR1B = (_BV(RXCIE1) | _BV(TXEN1) | _BV(RXEN1));
    }
}
const USB_Descriptor_Device_t PROGMEM DeviceDescriptor =
    {
        .Header = {.Size = sizeof(USB_Descriptor_Device_t), .Type = DTYPE_Device},

        .USBSpecification = VERSION_BCD(1, 1, 0),
        .Class = 0x02,
        .SubClass = 0x00,
        .Protocol = 0x00,

        .Endpoint0Size = FIXED_CONTROL_ENDPOINT_SIZE,

        .VendorID = 0x1209,

        .ProductID = 0x2883,
        .ReleaseNumber = 0x0001,

        .ManufacturerStrIndex = NO_DESCRIPTOR,
        .ProductStrIndex = NO_DESCRIPTOR,
        .SerialNumStrIndex = NO_DESCRIPTOR,

        .NumberOfConfigurations = FIXED_NUM_CONFIGURATIONS};

const USB_Descriptor_Configuration_t PROGMEM ConfigurationDescriptor =
    {
        .Config =
            {
                .Header = {.Size = sizeof(USB_Descriptor_Configuration_Header_t), .Type = DTYPE_Configuration},

                .TotalConfigurationSize = sizeof(USB_Descriptor_Configuration_t),
                .TotalInterfaces = 2,

                .ConfigurationNumber = 1,
                .ConfigurationStrIndex = NO_DESCRIPTOR,

                .ConfigAttributes = 0,

                .MaxPowerConsumption = USB_CONFIG_POWER_MA(100)},

        .CDC_CCI_Interface =
            {
                .Header = {.Size = sizeof(USB_Descriptor_Interface_t), .Type = DTYPE_Interface},

                .InterfaceNumber = 0,
                .AlternateSetting = 0,

                .TotalEndpoints = 1,

                .Class = 0x02,
                .SubClass = 0x02,
                .Protocol = 0x01,

                .InterfaceStrIndex = NO_DESCRIPTOR},

        .CDC_Functional_Header =
            {
                .Header = {.Size = sizeof(USB_CDC_Descriptor_FunctionalHeader_t), .Type = CDC_DTYPE_CSInterface},
                .Subtype = CDC_DSUBTYPE_CSInterface_Header,

                .CDCSpecification = VERSION_BCD(1, 1, 0),
            },

        .CDC_Functional_ACM =
            {
                .Header = {.Size = sizeof(USB_CDC_Descriptor_FunctionalACM_t), .Type = CDC_DTYPE_CSInterface},
                .Subtype = CDC_DSUBTYPE_CSInterface_ACM,

                .Capabilities = 0x06,
            },

        .CDC_Functional_Union =
            {
                .Header = {.Size = sizeof(USB_CDC_Descriptor_FunctionalUnion_t), .Type = CDC_DTYPE_CSInterface},
                .Subtype = CDC_DSUBTYPE_CSInterface_Union,

                .MasterInterfaceNumber = 0,
                .SlaveInterfaceNumber = 1,
            },

        .CDC_NotificationEndpoint =
            {
                .Header = {.Size = sizeof(USB_Descriptor_Endpoint_t), .Type = DTYPE_Endpoint},

                .EndpointAddress = CDC_NOTIFICATION,
                .Attributes = (EP_TYPE_INTERRUPT | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
                .EndpointSize = ENDPOINT_SIZE,
                .PollingIntervalMS = 0xFF},

        .CDC_DCI_Interface =
            {
                .Header = {.Size = sizeof(USB_Descriptor_Interface_t), .Type = DTYPE_Interface},

                .InterfaceNumber = 1,
                .AlternateSetting = 0,

                .TotalEndpoints = 2,

                .Class = 0x0A,
                .SubClass = 0x00,
                .Protocol = 0x00,

                .InterfaceStrIndex = NO_DESCRIPTOR},

        .CDC_DataOutEndpoint =
            {
                .Header = {.Size = sizeof(USB_Descriptor_Endpoint_t), .Type = DTYPE_Endpoint},

                .EndpointAddress = DEVICE_EPADDR_OUT,
                .Attributes = (EP_TYPE_BULK | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
                .EndpointSize = SERIAL_ENDPOINT_SIZE,
                .PollingIntervalMS = 0x01},

        .CDC_DataInEndpoint =
            {
                .Header = {.Size = sizeof(USB_Descriptor_Endpoint_t), .Type = DTYPE_Endpoint},

                .EndpointAddress = DEVICE_EPADDR_IN,
                .Attributes = (EP_TYPE_BULK | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
                .EndpointSize = SERIAL_ENDPOINT_SIZE,
                .PollingIntervalMS = 0x01}};