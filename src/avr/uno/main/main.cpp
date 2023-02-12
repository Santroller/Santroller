#include <Arduino.h>
#include <SPI.h>
#include <avr/io.h>
#include <util/delay.h>

#include "Usb.h"
#include "config.h"
#include "controller_reports.h"
#include "defines.h"
#include "descriptors.h"
#include "packets.h"
#include "shared_main.h"

// Set up some arrays for storing received data / data to transmit
uint8_t buf[255];

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

// reset_usb is called by other parts of the code if we need to trigger a usb reload
// we just trigger a flag here, as we can send it on our next usb transfer
bool should_reload_usb = false;
void reset_usb(void) {
    should_reload_usb = true;
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
}
USB_Report_Data_t report;
void loop() {
    // Wait for a packet from the 8u2/16u2.
    if (!ready) return;
    ready = false;
    // Now handle the packet
    switch (header->id) {
        case CONTROLLER_DATA_TRANSMIT_ID:
            // We received a hid out request, we need to process it
            hid_set_report(dt->data, header->len - sizeof(packet_header_t), INTERRUPT_ID, INTERRUPT_ID);
            header->len = 0;
            break;
        case CONTROLLER_DATA_REQUEST_ID: {
            header->len = 0;
            uint8_t len = tick_inputs(&report);
            // Write the controller input data
            memcpy(buf + sizeof(packet_header_t), &report, len);
            header->len += len;
            break;
        }
        case DESCRIPTOR_ID: {
            // 8u2/16u2 wants a descriptor, so return it
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
            // 8u2/16u2 wants us to handle a control request.
            uint16_t len = controlRequest(ctr->bmRequestType, ctr->request, ctr->wValue, ctr->wIndex, ctr->wLength, dt->data);
            if (len > ctr->wLength) len = ctr->wLength;
            header->len = len;
            break;
        }
        case DEVICE_ID:
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
