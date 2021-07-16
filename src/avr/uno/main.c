#include "lib_main.h"
// if jmpToBootloader is set to JUMP, then the arduino will jump to bootloader
// mode after the next watchdog reset
uint32_t jmpToBootloader __attribute__((section(".noinit")));

int main(void) {
    // // jump to the bootloader at address 0x1000 if jmpToBootloader is set to JUMP
    // if (jmpToBootloader == JUMP) {
    //     // We don't want to jump again after the bootloader returns control flow to
    //     // us
    //     jmpToBootloader = 0;
    //     asm volatile("jmp 0x1000");
    // }

    // /* Disable watchdog if enabled by bootloader/fuses */
    // MCUSR &= ~(1 << WDRF);
    // wdt_disable();

    // Serial_InitInterrupt(BAUD, true);

    // USB_Init();
    init();
    while (true) {
        tick();
    }
}