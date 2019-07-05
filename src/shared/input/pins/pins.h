#pragma once
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdint.h>
#include "../../arduino.h"

int digitalRead(uint8_t pin);
int analogRead(uint8_t pin);
void pinMode(uint8_t pin, uint8_t mode);
void enableADC(void);
void enablePCI(uint8_t pin);