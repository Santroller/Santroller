#include <Arduino.h>
#include <Wire.h>
#include <avr/io.h>

#include "Usb.h"
#include "controller_reports.h"
#include "defines.h"
#include "descriptors.h"
#include "packets.h"
#include "config.h"

uint8_t* bufTx;
uint8_t idx_tx = 0;
uint8_t buf[255];
uint8_t controller[48];
volatile bool ready = false;
packet_header_t* header = (packet_header_t*)buf;
descriptor_request_t* desc = (descriptor_request_t*)buf;
control_request_t* ctr = (control_request_t*)buf;
data_transmit_packet_t* dt = (data_transmit_packet_t*)buf;
void reset_usb(void) {
    // TODO: how do we handle this on the uno?
    // Maybe CONTROLLER_DATA_REQUEST_ID can store some sort of keepalive as the first byte, and if it isnt set then we restart usb?
}
#define SERIAL_2X_UBBRVAL(Baud) ((((F_CPU / 8) + (Baud / 2)) / (Baud)) - 1)

void setup() {
    UBRR0 = SERIAL_2X_UBBRVAL(BAUD);

    UCSR0C = ((1 << UCSZ01) | (1 << UCSZ00));
    UCSR0A = (1 << U2X0);
    UCSR0B = ((1 << TXEN0) | (1 << RXCIE0) | (1 << RXEN0));

    DDRD |= (1 << 3);
    PORTD |= (1 << 2);
    sei();
    UDR0 = READY;
    DDRB |= (1 << PB5);
    PORTB |= (1 << PB5);
}
void loop() {
    if (!ready) return;
    ready = false;
    switch (header->id) {
        case CONTROLLER_DATA_TRANSMIT_ID:
            header->len = sizeof(packet_header_t);
            // Technically, we could get multiple packets at once, so we should probably loop through them
            // packetReceived(dt->data, header->len - sizeof(packet_header_t));
            break;
        case CONTROLLER_DATA_REQUEST_ID: {
            // uint8_t clen = tick(controller);
            uint8_t clen = 0;
            header->len = sizeof(packet_header_t) + clen;
            memcpy(buf + sizeof(packet_header_t), controller, clen);
            break;
        }
        case DESCRIPTOR_ID: {
            uint16_t len = descriptorRequest(desc->wValue, desc->wIndex, buf + sizeof(packet_header_t));
            header->len = sizeof(packet_header_t) + len;
            break;
        }
        case CONTROL_REQUEST_ID: {
            bool valid = false;
            uint16_t len = 0;
            // uint16_t len2 = controlRequest(ctr->bmRequestType, ctr->request, ctr->wValue, ctr->wIndex, ctr->wLength, (buf + sizeof(packet_header_t) + 1), &valid);
            // if (len2 > ctr->wLength) len2 = ctr->wLength;
            header->len = sizeof(packet_header_t) + len + 1;
            buf[sizeof(packet_header_t)] = valid;
            break;
        }
        case DEVICE_ID:
            header->len = sizeof(packet_header_t) + 2;
            buf[sizeof(packet_header_t)] = deviceType;
            buf[sizeof(packet_header_t)+1] = consoleType;
            break;
        default:
            return;
    }
    bufTx = buf;
    idx_tx = header->len;
    UCSR0B = (_BV(RXCIE0) | _BV(TXEN0) | _BV(RXEN0) | _BV(UDRIE0));
}

uint8_t idx = 0;
ISR(USART_RX_vect, ISR_BLOCK) {
    buf[idx] = UDR0;
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

ISR(USART_UDRE_vect, ISR_BLOCK) {
    if (idx_tx--) {
        UDR0 = *(bufTx++);
    } else {
        UCSR0B = (_BV(RXCIE0) | _BV(TXEN0) | _BV(RXEN0));
    }
}