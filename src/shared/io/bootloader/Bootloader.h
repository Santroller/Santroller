#pragma once
#include <stdbool.h>
#include <util/delay.h>
#ifdef __cplusplus
extern "C" {
#endif
extern void bootloader(void);
extern void check_freq(void);
extern void serial(void);
extern bool check_serial(void);
extern void reboot(void);
extern void serial_jump_init(void);
#ifdef __cplusplus
}
#endif