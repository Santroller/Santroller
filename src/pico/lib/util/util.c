#include <hardware/sync.h>
#include <stdint.h>
#include <hardware/timer.h>
#include <pico/stdlib.h>
#include <pico/time.h>
uint32_t saved_irq;
void cli() { saved_irq = save_and_disable_interrupts(); }
void sei() { restore_interrupts(saved_irq); }
void _delay_ms(uint32_t __ms) { sleep_ms(__ms); }
void _delay_us(uint32_t __us) { sleep_us(__us); }