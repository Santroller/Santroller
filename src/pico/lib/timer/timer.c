#include "util/util.h"
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