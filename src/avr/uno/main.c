#include <LUFA/Drivers/Peripheral/Serial.h>

#include "lib_main.h"
#include "packets.h"
#include "usb.h"
#include "usb/controller_reports.h"

uint8_t* bufTx;
uint8_t idx_tx = 0;
uint8_t buf[255];
volatile bool ready = false;
packet_header_t* header = (packet_header_t*)buf;
descriptor_request_t* desc = (descriptor_request_t*)buf;
control_request_t* ctr = (control_request_t*)buf;
int main(void) {
    init();
    Serial_InitInterrupt(BAUD, true);
    sei();
    Serial_SendByte(READY);
    USB_XInputReport_Data_t out = {0, sizeof(USB_XInputReport_Data_t), 0};
    while (true) {
        tick();
        if (!ready) continue;
        __asm volatile("" ::
                           : "memory");
        ready = false;
        switch (header->id) {
            case CONTROLLER_DATA_TRANSMIT_ID:
                header->len = sizeof(packet_header_t);
                break;
            case CONTROLLER_DATA_REQUEST_ID:
                header->len = sizeof(packet_header_t) + sizeof(USB_XInputReport_Data_t);
                out.l_x += 0xFF;
                memcpy(buf + sizeof(packet_header_t), &out, sizeof(USB_XInputReport_Data_t));
                break;
            case DESCRIPTOR_ID:
                const void* data;
                uint16_t len = descriptorRequest(desc->wValue, desc->wIndex, &data);
                header->len = sizeof(packet_header_t) + len;
                if (len) {
                    memcpy(buf + sizeof(packet_header_t), data, len);
                }
                break;
            case CONTROL_REQUEST_ID:
                uint8_t* dataPtr;

                bool valid;
                uint16_t len2 = controlRequest(ctr->requestType, ctr->request, ctr->wValue, ctr->wIndex, ctr->wLength, &dataPtr, &valid);
                header->len = sizeof(packet_header_t) + len2 + 1;
                buf[sizeof(packet_header_t)] = valid;
                if (len2) {
                    memcpy(buf + sizeof(packet_header_t) + 1, dataPtr, len2);
                }
                break;
            case DEVICE_ID:
                header->len = sizeof(packet_header_t) + 1;
                buf[sizeof(packet_header_t)] = deviceType;
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

ISR(USART1_UDRE_vect, ISR_BLOCK) {
    if (idx_tx--) {
        UDR1 = *(bufTx++);
    } else {
        UCSR1B = (_BV(RXCIE1) | _BV(TXEN1) | _BV(RXEN1));
    }
}