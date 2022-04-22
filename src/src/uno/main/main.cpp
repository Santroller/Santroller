#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <avr/io.h>

#include "Usb.h"
#include "config.h"
#include "controller_reports.h"
#include "defines.h"
#include "descriptors.h"
#include "packets.h"
#include "controller_reports.h"

uint8_t* bufTX;
uint8_t idxTX = 0;
uint8_t buf[255];
uint8_t controller[48];
volatile bool ready = false;
packet_header_t* header = (packet_header_t*)buf;
descriptor_request_t* desc = (descriptor_request_t*)buf;
control_request_t* ctr = (control_request_t*)buf;
data_transmit_packet_t* dt = (data_transmit_packet_t*)buf;
bool should_reboot = false;
void reset_usb(void) {
    should_reboot = true;
}

void setup() {
    UBRR0 = SERIAL_2X_UBBRVAL(BAUD);
    UCSR0C = ((1 << UCSZ01) | (1 << UCSZ00));
    UCSR0A = (1 << U2X0);
    UCSR0B = ((1 << TXEN0) | (1 << RXCIE0) | (1 << RXEN0));
    sei();
    UDR0 = READY;
}
void loop() {
    if (!ready) return;
    ready = false;
    switch (header->id) {
        case CONTROLLER_DATA_TRANSMIT_ID:
            // Technically, we could get multiple packets at once, so we should probably loop through them
            // packetReceived(dt->data, header->len - sizeof(packet_header_t));
            header->len = 0;
            break;
        case CONTROLLER_DATA_REQUEST_ID: {
            header->len = 0;
            if (should_reboot) {
                header->id = CONTROLLER_DATA_REBOOT_ID;
            } else {
                memset(dt->data, 0, sizeof(USB_XInputReport_Data_t));
                USB_XInputReport_Data_t* xinput = (USB_XInputReport_Data_t*)dt->data;
                xinput->rid=0;
                xinput->rsize = sizeof(XInput_Data_t);
                xinput->r_x = rand();
                header->len = sizeof(USB_XInputReport_Data_t);
                // uint8_t clen = tick(controller);
                // memcpy(buf + sizeof(packet_header_t), controller, clen);
                // header->len += clen;
            }
            break;
        }
        case DESCRIPTOR_ID: {
            uint16_t wLength = desc->wLength;
            uint16_t len = descriptorRequest(desc->wValue, desc->wIndex, dt->data);
            if (len > wLength) len = wLength;
            header->len = len;
            break;
        }
        case CONTROL_REQUEST_VALIDATION_ID: {
            bool valid = controlRequestValid(ctr->bmRequestType, ctr->request, ctr->wValue, ctr->wIndex, ctr->wLength);
            dt->data[0] = valid;
            header->len = 1;
            break;
        }
        case CONTROL_REQUEST_ID: {
            bool valid = false;
            uint16_t len = controlRequest(ctr->bmRequestType, ctr->request, ctr->wValue, ctr->wIndex, ctr->wLength, &dt->data[1], &valid);
            if (len > ctr->wLength) len = ctr->wLength;
            header->len = len;
            if ((ctr->bmRequestType & USB_SETUP_DEVICE_TO_HOST) == USB_SETUP_HOST_TO_DEVICE) {
                header->len = 0;
            }

            break;
        }
        case DEVICE_ID:
            header->len = 1;
            buf[sizeof(packet_header_t)] = consoleType;
            break;
        default:
            return;
    }
    bufTX = buf;
    idxTX = header->len + sizeof(packet_header_t);
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
    if (idxTX--) {
        UDR0 = *(bufTX++);
    } else {
        UCSR0B = (_BV(RXCIE0) | _BV(TXEN0) | _BV(RXEN0));
    }
}