#include <stdint.h>
#include <hardware/sync.h>
uint32_t saved_irq;
void cli() { saved_irq = save_and_disable_interrupts(); }
void sei() { restore_interrupts(saved_irq); }