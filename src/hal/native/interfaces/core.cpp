
#include <interfaces/core.hpp>

void Core::go_to_sleep() {
    // set flag and restart, picos main func will handle this
}
void Core::read_serial(char* id, uint8_t len) {
    id[0] = 'a';
}
void Core::delay(unsigned long ms) {
    
}
void Core::delayMicroseconds(unsigned int usec) {
    
}
long Core::millis() {
    return 0;
}
long Core::micros() {
    return 0;
}