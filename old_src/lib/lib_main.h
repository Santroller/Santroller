#pragma once
#include <stdbool.h>
#include <stdint.h>

#include "config.h"
#include "defines.h"
#include "descriptors.h"
#include "pin.h"
#ifndef _BV
#define _BV(bit) (1 << (bit))
#endif
#define bit_set(p, m) ((p) |= _BV(m))
#define bit_clear(p, m) ((p) &= ~_BV(m))
// #define bit_write(c, p, m) (c ? bit_set(p, m) : bit_clear(p, m))
#define bit_write(c, p, m) ((p) = (p & ~(_BV(m))) | ((!!(c)) << (m)));
#define bit_check(value, bit) (value & _BV(bit))
extern ConsoleType_t consoleType;
extern DeviceType_t deviceType;
extern bool guitar;
extern bool drum;
extern bool read_hid_report_descriptor;
extern uint8_t pinCount;
extern uint8_t inputCount;
extern Input_t pins[PORTS * PINS_PER_PORT];
extern AnalogInput_t analogInfo[NUM_ANALOG_INPUTS];
extern uint8_t buf[255];
extern uint8_t firstKeyboardPin;

void init(void);
uint8_t tick(uint8_t* data);
void packetReceived(uint8_t* data, uint8_t len);
void initPins(void);
unsigned long millis(void);
unsigned long micros(void);
void reset_usb(void);