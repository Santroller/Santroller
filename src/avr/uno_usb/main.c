#include <LUFA/Drivers/Board/Board.h>
#include <LUFA/Drivers/Board/LEDs.h>
#include <LUFA/Drivers/Peripheral/Serial.h>
#include <LUFA/Drivers/USB/Class/CDCClass.h>
#include <LUFA/Drivers/USB/USB.h>
#include <avr/wdt.h>

#include "packets.h"
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
    // _delay_ms(2000);
    AVR_RESET_LINE_DDR |= AVR_RESET_LINE_MASK;
    AVR_RESET_LINE_PORT |= AVR_RESET_LINE_MASK;
    sei();
    while (waitingForBoot) {
    }
    USB_Init();
    while (true) {
        USB_USBTask();
    }
}
void EVENT_USB_Device_ControlRequest(void) {
    // pass request to 328p
    // If we always know the size of the request, could we just avoid using circular buffers altogether, and just use a standard array?
    // As long as we are always syncronising packets so that there is only ever one packet in transit on either pipe, this should just work.
    // This does become an issue when we do uno programming, but we will have to think about that when it becomes an issue.
    // In theory though, the STK500 commands have a fixed length too, so we dont have an issue there
    // This also means we can flick the speed to 115200 for a programming command and then flick it back
}
void EVENT_USB_Device_ConfigurationChanged(void) {
    // TODO: change to EP_TYPE_BULK for midi (we will probably need a command just to get the type for this)
    // Endpoint_ConfigureEndpoint(DEVICE_EPADDR_IN, EP_TYPE_INTERRUPT, HID_EPSIZE_IN,
    //                            1);
    // Endpoint_ConfigureEndpoint(DEVICE_EPADDR_OUT, EP_TYPE_INTERRUPT, HID_EPSIZE_OUT,
    //                            1);
}

uint16_t CALLBACK_USB_GetDescriptor(const uint16_t wValue,
                                    const uint16_t wIndex,
                                    const void** const descriptorAddress) {
    // pass request to 328p
    descriptor_request_t packet = {
        header : DESC_REQUEST_HEADER,
        wValue : wValue,
        wIndex : wIndex
    };
    Serial_SendData(&packet, sizeof(descriptor_request_t));
    while (!ready) {
    }
    asm volatile("" ::
                     : "memory");
    ready = false;
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
    if (idx >= sizeof(packet_header_t) && header->len == idx) {
        idx = 0;
        ready = true;
        return;
    }
}