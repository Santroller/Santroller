#pragma once
#include "controller/controller.h"
#include "pins_arduino.h"
#include "util/util.h"
// #include <avr/interrupt.h>
// #include <avr/pgmspace.h>
#include <stdbool.h>
#include <stdint.h>
// The pico and avr devices are fundamentally different. We use direct memory access on avr, but just need a pin number for the pico
#ifdef __AVR__
typedef struct {
  uint8_t mask;
  volatile uint8_t *port;
  bool eq;
  uint8_t pin;
  uint8_t offset;
  uint32_t lastMillis;
  uint8_t milliDeBounce;
  uint8_t analogOffset;
} Pin_t;
#else
typedef struct {
  uint8_t mask;
  uint16_t pmask;
  uint16_t outmask;
  bool eq;
  uint8_t offset;
  uint8_t pin;
  uint32_t lastMillis;
  uint8_t milliDeBounce;
  bool sioFunc;
  uint8_t analogOffset;
} Pin_t;
#endif
typedef struct {
  uint8_t offset;
  uint8_t pin;
  bool inverted;
  volatile int16_t value;
  uint16_t threshold;
  // Does this analog pin map to a digital pin (aka drums)
  bool hasDigital;
} AnalogInfo_t;

extern AnalogInfo_t joyData[NUM_ANALOG_INPUTS];
extern int validAnalog;
#define LOW 0
#define CHANGE 1
#define FALLING 2
#define RISING 3
void setUpAnalogPin(Configuration_t* config, uint8_t pin);
bool digitalRead(uint8_t pin);
bool digitalReadPin(Pin_t pin);
void pinMode(uint8_t pin, uint8_t mode);
void setupADC(void);
void tickAnalog(void);
uint16_t analogRead(uint8_t pin);
void stopReading(void);
void setUpValidPins(Configuration_t* config);
void setUpAnalogDigitalPin(Pin_t* button, uint8_t pin, uint16_t threshold);
Pin_t setUpDigital(Configuration_t* config, uint8_t pin, uint8_t offset, bool inverted, bool output);
void digitalWritePin(Pin_t pin, bool value);
void digitalWrite(uint8_t pin, uint8_t value);