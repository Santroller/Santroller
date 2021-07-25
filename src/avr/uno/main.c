#include <LUFA/Drivers/Peripheral/Serial.h>

#include "lib_main.h"
#include "packets.h"
#include "usb.h"
#include "usb/controller_reports.h"
#include <stdlib.h>
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
    USB_XInputReport_Data_t out = {0, sizeof(USB_XInputReport_Data_t), 0};
    while (true) {
        tick();
        if (Serial_IsCharReceived()) {
            buf[index] = Serial_ReceiveByte();
            if (index == 0 && buf[0] != VALID_PACKET) {
                continue;
            }
            index++;
            if (index >= sizeof(packet_header_t)) {
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
                            out.l_x = rand();
                            Serial_SendData(buf, sizeof(packet_header_t));
                            Serial_SendData(&out, sizeof(USB_XInputReport_Data_t));
                            break;
                        case CONTROLLER_DATA_TRANSMIT_ID:
                            header->len = sizeof(packet_header_t);
                            Serial_SendData(buf, sizeof(packet_header_t));
                            break;
                    }
                    index = 0;
                }
            }
        }
    }
}