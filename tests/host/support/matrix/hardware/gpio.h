#pragma once
#include <stdint.h>

#define NUM_BANK0_GPIOS 48

void gpio_init_mask64(uint64_t mask);
void gpio_set_dir_in_masked64(uint64_t mask);
void gpio_set_dir_out_masked64(uint64_t mask);
void gpio_put_masked64(uint64_t mask, uint64_t value);
void gpio_set_pulls(unsigned int pin, bool up, bool down);
bool gpio_get(unsigned int pin);
