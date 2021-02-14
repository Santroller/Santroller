#include "bootloader/bootloader.h"
#include "util/util.h"
#include "pico/bootrom.h"
#include "hardware/watchdog.h"
void reboot(void) {
    watchdog_enable(1, false);
    for (;;) {}
}
void bootloader(void) {
   reset_usb_boot(0,0);
}