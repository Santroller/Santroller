#include <LUFA/Drivers/Peripheral/Serial.h>

#include "lib_main.h"
#include "packets.h"
#include "usb.h"
#include "usb/controller_reports.h"
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
        __asm volatile( "" ::: "memory" );
        ready = false;
        switch (header->id) {
            case DESCRIPTOR_ID:
                const void* data;
                uint16_t len = descriptorRequest(desc->wValue, desc->wIndex, &data);
                header->len = sizeof(packet_header_t) + len;
                Serial_SendData(buf, sizeof(packet_header_t));
                if (len) {
                    Serial_SendData(data, len);
                }
                break;
            case CONTROL_REQUEST_ID:
                uint8_t* dataPtr;

                bool valid;
                uint16_t len2 = controlRequest(ctr->requestType, ctr->request, ctr->wValue, ctr->wIndex, ctr->wLength, &dataPtr, &valid);
                header->len = sizeof(packet_header_t) + len2 + 1;
                Serial_SendData(buf, sizeof(packet_header_t));
                Serial_SendByte(valid);
                if (len2) {
                    Serial_SendData(dataPtr, len2);
                }
                break;
            case DEVICE_ID:
                header->len = sizeof(packet_header_t) + 1;
                Serial_SendData(buf, sizeof(packet_header_t));
                Serial_SendByte(deviceType);
                break;
            case CONTROLLER_DATA_REQUEST_ID:
                header->len = sizeof(packet_header_t) + sizeof(USB_XInputReport_Data_t);
                out.l_x += 1000;
                Serial_SendData(buf, sizeof(packet_header_t));
                Serial_SendData(&out, sizeof(USB_XInputReport_Data_t));
                break;
            case CONTROLLER_DATA_TRANSMIT_ID:
                header->len = sizeof(packet_header_t);
                Serial_SendData(buf, sizeof(packet_header_t));
                break;
        }
    }
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