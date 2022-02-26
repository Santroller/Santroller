#include <Arduino.h>
#if defined(__MK20DX128__) || defined(__MK20DX256__) || defined(__MKL26Z64__) || \
    defined(__MK64FX512__) || defined(__MK66FX1M0__) // Teensy 3.0/3.1-3.2/LC/3.5/3.6
#include <i2c_t3.h>
#else
#include <Wire.h>  // All other platforms
#endif
#include <SPI.h>
void setup() {
	delay(2000);
}

void loop() {
	delay(2000);
}