#pragma once
#include <stdbool.h>
#include <util/delay.h>
extern void bootloader(void);
extern void serial(void);
extern bool check_serial(void);
extern void reboot(void);
extern void serial_jump_init(void);