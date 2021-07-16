#include "lib_main.h"
#include "serial.h"
#include "defines.h"
uint8_t deviceType;
bool guitar;
bool drum;
uint16_t serialString[SERIAL_LEN];
void init() {
    generateSerialString(serialString);
    deviceType = XINPUT_GUITAR_HERO_GUITAR;
}
void tick() {
    
}