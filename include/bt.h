#pragma once
#include <stdbool.h>
int btstack_main();
void send_report(uint8_t size, uint8_t* report);
bool check_bluetooth_ready(void);