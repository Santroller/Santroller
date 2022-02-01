#include <LUFA/Drivers/Peripheral/Serial.h>
#include <avr/io.h>

#include "defines.h"
#include "lib_main.h"
#include "packets.h"
#include "usb.h"
#include "usb/controller_reports.h"
#include "avr.h"

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
}
int main(void) {
    init();
    setupMicrosTimer();
    Serial_InitInterrupt(BAUD, true);
    sei();
    Serial_SendByte(READY);
    DDRB |= (1 << PB5);
    PORTB |= (1 << PB5);
    // Ensure the 328p is reset so we can also work from bootloader
    while (true) {
        if (!ready) continue;
        __asm volatile("" ::
                           : "memory");
        ready = false;
        switch (header->id) {
            case CONTROLLER_DATA_TRANSMIT_ID:
                header->len = sizeof(packet_header_t);
                // Technically, we could get multiple packets at once, so we should probably loop through them
                packetReceived(dt->data, header->len - sizeof(packet_header_t));
                break;
            case CONTROLLER_DATA_REQUEST_ID:
                uint8_t clen = tick(controller);
                header->len = sizeof(packet_header_t) + clen;
                memcpy(buf + sizeof(packet_header_t), controller, clen);
                break;
            case DESCRIPTOR_ID:
                uint16_t len = descriptorRequest(desc->wValue, desc->wIndex, buf + sizeof(packet_header_t));
                header->len = sizeof(packet_header_t) + len;
                break;
            case CONTROL_REQUEST_ID:
                bool valid;
                uint16_t len2 = controlRequest(ctr->requestType, ctr->request, ctr->wValue, ctr->wIndex, ctr->wLength, (buf + sizeof(packet_header_t) + 1), &valid);
                if (len2 > ctr->wLength) len2 = ctr->wLength;
                header->len = sizeof(packet_header_t) + len2 + 1;
                buf[sizeof(packet_header_t)] = valid;
                break;
            case DEVICE_ID:
                header->len = sizeof(packet_header_t) + 2;
                buf[sizeof(packet_header_t)] = deviceType;
                buf[sizeof(packet_header_t)+1] = consoleType;
                break;
            default:
                continue;
        }
        bufTx = buf;
        idx_tx = header->len;
        UCSR1B = (_BV(RXCIE1) | _BV(TXEN1) | _BV(RXEN1) | _BV(UDRIE1));
    }
}

uint8_t idx = 0;
ISR(USART1_RX_vect, ISR_BLOCK) {
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