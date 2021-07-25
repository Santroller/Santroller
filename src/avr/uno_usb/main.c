#include <LUFA/Drivers/Board/Board.h>
#include <LUFA/Drivers/Board/LEDs.h>
#include <LUFA/Drivers/Peripheral/Serial.h>
#include <LUFA/Drivers/USB/Class/CDCClass.h>
#include <LUFA/Drivers/USB/USB.h>
#include <avr/wdt.h>

#include "defines.h"
#include "descriptors.h"
#include "packets.h"
#include "stk500_commands.h"
#define JUMP 0xDEAD8001
// if jmpToBootloader is set to JUMP, then the arduino will jump to bootloader
// mode after the next watchdog reset
uint32_t jmpToBootloader __attribute__((section(".noinit")));

// TODO: how big does this need to be?
uint8_t buf[255];
packet_header_t* header = (packet_header_t*)buf;
ret_packet_t* ret = (ret_packet_t*)buf;
volatile bool ready = false;
volatile bool waitingForBoot = true;
volatile bool stk500Mode = false;
volatile uint16_t stk500Len = 0;

void txRX(void* data, uint16_t len) {
    Serial_SendData(data, len);
    while (!ready) {
    }
    ready = false;
}
int main(void) {
    // jump to the bootloader at address 0x1000 if jmpToBootloader is set to JUMP
    if (jmpToBootloader == JUMP) {
        // We don't want to jump again after the bootloader returns control flow to
        // us
        jmpToBootloader = 0;
        asm volatile("jmp 0x1000");
    }

    /* Disable watchdog if enabled by bootloader/fuses */
    MCUSR &= ~(1 << WDRF);
    wdt_disable();

    Serial_InitInterrupt(BAUD, true);
    AVR_RESET_LINE_DDR |= AVR_RESET_LINE_MASK;
    AVR_RESET_LINE_PORT |= AVR_RESET_LINE_MASK;
    sei();
    // Wait for the 328p / 1280 to boot
    while (waitingForBoot) {
    }
    USB_Init();
    packet_header_t requestData = {
        VALID_PACKET, CONTROLLER_DATA_REQUEST_ID, sizeof(packet_header_t)};
    while (true) {
        USB_USBTask();
        if (USB_DeviceState != DEVICE_STATE_Configured)
            continue;
        Endpoint_SelectEndpoint(DEVICE_EPADDR_IN);
        if (Endpoint_IsINReady()) {
            txRX(&requestData, sizeof(packet_header_t));
            Endpoint_Write_Stream_LE(ret->data, ret->header.len - sizeof(packet_header_t), NULL);
            Endpoint_ClearIN();
        }
        // TODO: we will need to work out the length of packets. thats going to be different for different consoles..

        // data_transmit_packet_t* dt = data_transmit_packet_t * buf;
        // Endpoint_SelectEndpoint(CONTROLLER_DATA_TRANSMIT_ID);
        // if (Endpoint_IsOUTReceived()) {
        //     packet->header.magic = VALID_PACKET;
        //     packet->header.id = CONTROL_REQUEST_ID;
        //     packet->header.len = sizeof(control_request_t);
        //     /* Check to see if the packet contains data */
        //     if (Endpoint_IsReadWriteAllowed()) {
        //         Endpoint_Read_Stream_LE(buf, header->len - sizeof(packet_header_t), NULL);
        //         txRX(&requestDataOut, sizeof(packet_header_t));
        //     }
        //     Endpoint_ClearOUT();
        // }
    }
}

void EVENT_USB_Device_ControlRequest(void) {
    if (!(Endpoint_IsSETUPReceived())) return;
    bool isStk500 = false;      //TODO: define how we know if we have a stk500 packet
    bool isBootloader = false;  //TODO: define how we know if we have reached the bootloader
    bool hostToDevice = ((requestType_t)USB_ControlRequest.bmRequestType).bmRequestType_bit.direction == USB_DIR_HOST_TO_DEVICE;
    if (isBootloader) {
        jmpToBootloader = JUMP;
        cli();
        wdt_enable(WDTO_15MS);
        for (;;) {
        }
    }
    if (isStk500) {
        Endpoint_ClearSETUP();
        stk500Len = USB_ControlRequest.wLength;
        // All read / writes for stk500 will have a write command and then a read command
        // We stay at the right baudrate for both commands, and the read command
        // Doesnt actually need to transmit anything as the data is read back as part of the write
        // command
        if (hostToDevice) {
            Serial_InitInterrupt(STK500_BAUD, true);
            Endpoint_Read_Control_Stream_LE(buf, USB_ControlRequest.wLength);
            txRX(buf, USB_ControlRequest.wLength);
        } else {
            Endpoint_Write_Control_Stream_LE(buf, USB_ControlRequest.wLength);
            Serial_InitInterrupt(BAUD, true);
        }
        Endpoint_ClearStatusStage();
        return;
    }
    control_request_t* packet = (control_request_t*)buf;
    packet->header.magic = VALID_PACKET;
    packet->header.id = CONTROL_REQUEST_ID;
    packet->header.len = sizeof(control_request_t);
    packet->requestType.bmRequestType = USB_ControlRequest.bmRequestType;
    packet->request = USB_ControlRequest.bRequest;
    packet->wValue = USB_ControlRequest.wValue;
    packet->wIndex = USB_ControlRequest.wIndex;
    packet->wLength = USB_ControlRequest.wLength;
    // TODO: we may want to have a list of valid requests, otherwise this will respond to requests that it really should not.
    if (hostToDevice && packet->requestType.bmRequestType_bit.type == USB_REQ_TYPE_VENDOR && packet->requestType.bmRequestType_bit.recipient == USB_REQ_RCPT_INTERFACE) {
        Endpoint_ClearSETUP();
        packet->header.len += USB_ControlRequest.wLength;
        Endpoint_Read_Control_Stream_LE(packet->data, USB_ControlRequest.wLength);
        Endpoint_ClearStatusStage();
    }
    txRX(packet, packet->header.len);
    if (ret->data[0] && !hostToDevice) {
        Endpoint_ClearSETUP();
        Endpoint_Write_Control_Stream_LE(ret->data + 1, ret->header.len - sizeof(packet_header_t) - 1);
        Endpoint_ClearStatusStage();
    }
}
void EVENT_USB_Device_ConfigurationChanged(void) {
    packet_header_t packet = {
        VALID_PACKET, DEVICE_ID, sizeof(packet_header_t)};
    txRX(&packet, sizeof(packet_header_t));
    uint8_t deviceType = ret->data[0];
    uint8_t type = EP_TYPE_INTERRUPT;
    if (deviceType >= MIDI_GAMEPAD) {
        type = EP_TYPE_BULK;
    }
    uint8_t sizein = HID_EPSIZE_IN;
    uint8_t sizeout = HID_EPSIZE_OUT;
    if (deviceType <= XINPUT_ARCADE_PAD) {
        sizein = XINPUT_EPSIZE_IN;
        sizeout = XINPUT_EPSIZE_OUT;
    }
    Endpoint_ConfigureEndpoint(DEVICE_EPADDR_IN, type, sizein, 1);
    Endpoint_ConfigureEndpoint(DEVICE_EPADDR_OUT, type, sizeout, 1);
}

uint16_t CALLBACK_USB_GetDescriptor(const uint16_t wValue,
                                    const uint16_t wIndex,
                                    const void** const descriptorAddress) {
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
    if (waitingForBoot) {
        if (UDR1 == READY) {
            waitingForBoot = false;
        }
        return;
    }
    buf[idx] = UDR1;
    if (idx == 0 && buf[0] != VALID_PACKET) {
        return;
    }
    idx++;
    // stk500 purely relies on the wLength of a packet, as we dont have headers with the length
    if (stk500Mode) {
        if (idx == stk500Len) {
            idx = 0;
            ready = true;
            return;
        }
    } else {
        if (idx >= sizeof(packet_header_t) && header->len == idx) {
            idx = 0;
            ready = true;
            return;
        }
    }
}