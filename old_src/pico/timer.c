#include <stdint.h>
#include <pico/time.h>
#include <pico/stdlib.h>
#include <hardware/timer.h>

unsigned long millis(void) {
  return us_to_ms(time_us_64());
}

unsigned long micros(void) {
  return time_us_64();
}

void setupMicrosTimer(void) {
}
void delay_ms(uint32_t __ms) {
    sleep_ms(__ms);
}
void delay_us(uint32_t __us) {
    sleep_us(__us);
}