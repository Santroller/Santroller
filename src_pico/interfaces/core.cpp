
#include <pico/time.h>

#include <interfaces/core.hpp>

#include "Arduino.h"
void Core::go_to_sleep() {
    // set flag and restart, picos main func will handle this
}
void Core::read_serial(char* id, uint8_t len) {
    pico_get_unique_board_id_string(id, sizeof(id));
}
void Core::delay(unsigned long ms) {
    sleep_ms(ms);
}
void Core::delayMicroseconds(unsigned int usec) {
    sleep_us(usec);
}