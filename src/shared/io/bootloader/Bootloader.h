#pragma once
#include <stdbool.h>
#include <util/delay.h>
#ifdef __cplusplus
extern "C" {
#endif
extern void bootloader(void);
extern void check_freq(void);
extern void serial(void);
extern bool check_serial();
#ifdef __cplusplus
}
#endif