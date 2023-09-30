#include <Arduino.h>
#include <SPI.h>
#include <avr/boot.h>
#include <avr/io.h>
#include <util/delay.h>

#include "Usb.h"
#include "commands.h"
#include "config.h"
#include "defines.h"
#include "descriptors.h"
#include "hid.h"
#include "packets.h"
#include "reports/controller_reports.h"
#include "shared_main.h"
#define INTERNAL_SERIAL_START_ADDRESS 0x0E
#define USB_STRING_LEN(UnicodeChars) (sizeof(USB_Descriptor_Header_t) + ((UnicodeChars) << 1))
#define INTERNAL_SERIAL_LENGTH_BITS 80
typedef struct
{
    uint8_t Size; /**< Size of the descriptor, in bytes. */
    uint8_t Type; /**< Type of the descriptor, either a value in \ref USB_DescriptorTypes_t or a value
                   *   given by the specific class.
                   */
} __attribute__((packed)) USB_Descriptor_Header_t;
typedef struct
{
    USB_Descriptor_Header_t Header;
    uint16_t UnicodeString[(INTERNAL_SERIAL_LENGTH_BITS / 4) + 3];
} __attribute__((packed)) SignatureDescriptor_t;

volatile uint16_t persistedConsoleType __attribute__((section(".noinit")));
volatile uint16_t persistedConsoleTypeValid __attribute__((section(".noinit")));
SignatureDescriptor_t signature;
// Set up some arrays for storing received data / data to transmit
uint8_t buf[255];
uint8_t buf2[255];

// Flag for if we have received a packet that needs to be processed
volatile bool ready = false;

// pointer to data to send, along with the length of the data remaining to send
uint8_t* bufTX;
uint8_t idxTX = 0;

// Some reinterpretations of the data receive / send array, so we can easily read and write packets
packet_header_t* header = (packet_header_t*)buf;
descriptor_request_t* desc = (descriptor_request_t*)buf;
control_request_t* ctr = (control_request_t*)buf;
data_transmit_packet_t* dt = (data_transmit_packet_t*)buf;

// If we have been ticked by usb for new data, then this will be true
bool has_previous_data = false;
uint8_t previous_data_len = 0;
// reset_usb is called by other parts of the code if we need to trigger a usb reload
// we just trigger a flag here, as we can send it on our next usb transfer
bool should_reload_usb = false;
bool usb_ready = false;

static void USB_Device_GetInternalSerialDescriptor(void) {
    signature.Header.Type = 0x03;
    signature.Header.Size = USB_STRING_LEN((INTERNAL_SERIAL_LENGTH_BITS / 4) + 3);

    uint8_t CurrentGlobalInt = SREG;
    cli();

    uint8_t SigReadAddress = INTERNAL_SERIAL_START_ADDRESS;

    for (uint8_t SerialCharNum = 0; SerialCharNum < (INTERNAL_SERIAL_LENGTH_BITS / 4); SerialCharNum++) {
        uint8_t SerialByte = boot_signature_byte_get(SigReadAddress);

        if (SerialCharNum & 0x01) {
            SerialByte >>= 4;
            SigReadAddress++;
        }

        SerialByte &= 0x0F;

        signature.UnicodeString[SerialCharNum] = (SerialByte >= 10) ? (('A' - 10) + SerialByte) : ('0' + SerialByte);
    }

    SREG = CurrentGlobalInt;
    signature.UnicodeString[(INTERNAL_SERIAL_LENGTH_BITS / 4)] = consoleType + '0';
    signature.UnicodeString[(INTERNAL_SERIAL_LENGTH_BITS / 4) + 1] = DEVICE_TYPE + '0';
    signature.UnicodeString[(INTERNAL_SERIAL_LENGTH_BITS / 4) + 2] = WINDOWS_USES_XINPUT + '0';
}

void reset_usb(void) {
    persistedConsoleType = consoleType;
    persistedConsoleTypeValid = 0x3A2F;
    should_reload_usb = true;
    USB_Device_GetInternalSerialDescriptor();
}

bool usb_configured(void) {
    return usb_ready;
}

void setup() {
    // Configure the UART for controller mode
    UBRR0 = SERIAL_2X_UBBRVAL(BAUD);
    UCSR0C = ((1 << UCSZ01) | (1 << UCSZ00));
    UCSR0A = (1 << U2X0);
    UCSR0B = ((1 << TXEN0) | (1 << RXCIE0) | (1 << RXEN0));
    interrupts();

    // Let the 8u2/16u2 know we are ready to receive data
    UDR0 = READY;
    init_main();
    if (persistedConsoleTypeValid == 0x3A2F) {
        consoleType = persistedConsoleType;
    }
    USB_Device_GetInternalSerialDescriptor();
}
bool ready_for_next_packet() {
    return !has_previous_data;
}
void send_report_to_pc(const void* report, uint8_t len) {
    // Write the controller input data
    memcpy(buf2, report, len);
    previous_data_len = len;
    has_previous_data = true;
}
void loop() {
    // Wait for a packet from the 8u2/16u2.
    if (!ready) {
        return;
    }
    ready = false;
    // Now handle the packet
    switch (header->id) {
        case USB_READY:
            usb_ready = dt->data[0];
            return;
        case CONTROLLER_DATA_TRANSMIT_ID:
            // We received a hid out request, we need to process it
            hid_set_report(dt->data, header->len - sizeof(packet_header_t), INTERRUPT_ID, INTERRUPT_ID);
            header->len = 0;
            break;
        case CONTROLLER_DATA_REQUEST_ID: {
            header->len = 0;
            tick();
            if (has_previous_data) {
                has_previous_data = false;
                memcpy(buf + sizeof(packet_header_t), buf2, previous_data_len);
                header->len = previous_data_len;
            }
            break;
        }
        case DESCRIPTOR_ID: {
            // 8u2/16u2 wants a descriptor, so return it
            // Serial number string descriptor, return one we prepared earlier
            if (desc->wValue == ((0x03 << 8) | 3)) {
                memcpy(dt->data, &signature, sizeof(SignatureDescriptor_t));
                header->len = sizeof(SignatureDescriptor_t);
                break;
            }
            // Wlength will be overwritten in descriptorRequest so cache it
            uint16_t wLength = desc->wLength;
            uint16_t len = descriptorRequest(desc->wValue, desc->wIndex, dt->data);
            if (len > wLength) len = wLength;
            header->len = len;
            break;
        }
        case CONTROL_REQUEST_VALIDATION_ID: {
            // 8u2/16u2 wants to know if we handle a descriptor, so let it know that
            bool valid = controlRequestValid(ctr->bmRequestType, ctr->request, ctr->wValue, ctr->wIndex, ctr->wLength);
            dt->data[0] = valid;
            header->len = 1;
            break;
        }
        case CONTROL_REQUEST_ID: {
            // Wlength will be overwritten in descriptorRequest so cache it
            uint16_t wLength = ctr->wLength;
            // Some of our commands really need up to date data, so do another poll.
            if (ctr->request >= COMMAND_REBOOT && ctr->request < MAX) {
                tick();
            }
            // 8u2/16u2 wants us to handle a control request.
            uint16_t len = controlRequest(ctr->bmRequestType, ctr->request, ctr->wValue, ctr->wIndex, ctr->wLength, dt->data);
            if (len > wLength) len = wLength;
            header->len = len;
            break;
        }
        case DEVICE_ID:
            usb_ready = true;
            // 8u2/16u2 wants to know the console type, so return that
            dt->data[0] = consoleType;
            header->len = 1;
            break;
        default:
            // unknown packet, do nothing
            return;
    }
    // We are reloading usb, so instead of sending data back, let the 8u2/16u2 know we want usb reset
    if (should_reload_usb) {
        header->id = CONTROLLER_DATA_RESTART_USB_ID;
        header->len = 0;
        should_reload_usb = false;
    }
    // Send back any data in the buffers. enable the interrupt for pushing out data
    bufTX = buf;
    idxTX = header->len + sizeof(packet_header_t);
    UCSR0B = (_BV(RXCIE0) | _BV(TXEN0) | _BV(RXEN0) | _BV(UDRIE0));
}

// Data receive interrupt
// Keep track of what we have read so far
uint8_t idx = 0;
ISR(USART_RX_vect, ISR_BLOCK) {
    buf[idx] = UDR0;
    // Wait for something that looks like the start of a packet
    if (idx == 0 && buf[0] != VALID_PACKET) {
        return;
    }
    // We found the start of a packet, so read the header and then use the length from that to read the entire packet into buf
    idx++;
    if (idx >= sizeof(packet_header_t) && header->len == idx) {
        // We are done. reset for the next packet, and let the main loop know there is something to do
        idx = 0;
        ready = true;
        return;
    }
}

// Data transmit interrupt
ISR(USART_UDRE_vect, ISR_BLOCK) {
    if (idxTX--) {
        // There is something to send still, so send that byte, and decrement the count of bytes
        UDR0 = *(bufTX++);
    } else {
        // We are out of things to send, disable this interrupt
        UCSR0B = (_BV(RXCIE0) | _BV(TXEN0) | _BV(RXEN0));
    }
}
