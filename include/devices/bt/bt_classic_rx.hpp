#pragma once
#include <stdint.h>
int btstack_classic_main(bool enable_hid_host);
void btstack_classic_set_accept_incoming(bool accept);
void btc_start_scan(uint32_t lap = 0x9E8B33);
void btc_stop_scan(void);