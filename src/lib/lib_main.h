#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "descriptors.h"
#include "defines.h"
#include "config.h"
#include "pin.h"
#define bit_set(p, m) ((p) |= _BV(m))
#define bit_clear(p, m) ((p) &= ~_BV(m))
// #define bit_write(c, p, m) (c ? bit_set(p, m) : bit_clear(p, m))
#define bit_write(c, p, m) ((p) = (p & ~(_BV(m))) | ((!!(c)) << m));
#define bit_check(value, bit) (value & _BV(bit))
extern ConsoleType_t consoleType;
extern DeviceType_t deviceType;
extern bool guitar;
extern bool drum;
extern uint8_t pinCount;
extern Pin_t pins[PORTS*PINS_PER_PORT];
extern AnalogPin_t analogInfo[NUM_ANALOG_INPUTS];
extern uint8_t buf[255];

void init(void);
uint8_t tick(uint8_t* data);
void packetReceived(uint8_t* data, uint8_t len);
void initPins(void);
unsigned long millis(void);
unsigned long micros(void);