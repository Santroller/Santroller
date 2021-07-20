#include <LUFA/Drivers/Peripheral/Serial.h>

#include "lib_main.h"
#include "packets.h"
#include "usb.h"
int main(void) {
    init();
    Serial_Init(BAUD, true);
    sei();
    Serial_SendByte(READY);
    uint8_t index = 0;
    uint8_t buf[255];
    packet_header_t* header = (packet_header_t*)buf;
    descriptor_request_t* desc = (descriptor_request_t*)buf;
    control_request_t* ctr = (control_request_t*)buf;
    while (true) {
        tick();
        if (Serial_IsCharReceived()) {
            buf[index] = Serial_ReceiveByte();
            if (index == 0 && buf[0] != VALID_PACKET) {
                continue;
            }
            index++;
            if (index > sizeof(packet_header_t)) {
                if (header->len == index) {
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
                            if (ctr->requestType.bmRequestType_bit.direction == USB_DIR_DEVICE_TO_HOST) {
                                if (controlRequest(ctr->requestType, ctr->request, ctr->wValue, ctr->wIndex, ctr->wLength, &dataPtr)) {
                                    header->len = sizeof(packet_header_t) + ctr->wLength;
                                    Serial_SendData(buf, sizeof(packet_header_t));
                                    Serial_SendData(dataPtr, ctr->wLength);
                                }
                            } else {
                                dataPtr = ctr->data;
                                controlRequest(ctr->requestType, ctr->request, ctr->wValue, ctr->wIndex, ctr->wLength, &dataPtr);
                            }
                            break;
                    }
                    index = 0;
                }
            }
        }
    }
}