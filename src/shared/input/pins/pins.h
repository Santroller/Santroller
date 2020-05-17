#pragma once
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdint.h>
#include <stdbool.h>
#include "controller/controller.h"
#include "util/util.h"
#include "arduino_pins.h"

typedef struct {
  uint8_t mask;
  volatile uint8_t *port;
  uint16_t pmask;
  bool eq;
  uint8_t offset;
} Pin_t;

typedef struct {
  uint8_t srb;
  uint8_t mux;
  uint8_t offset;
  uint8_t pin;
  bool inverted;
  volatile uint16_t value;
  uint16_t threshold;
  // Does this analog pin map to a digital pin (aka drums)
  bool hasDigital;
  Pin_t digital;
} AnalogInfo_t;

extern AnalogInfo_t joyData[NUM_ANALOG_INPUTS];
extern int validAnalog;

void setUpAnalogPin(uint8_t pin);
int digitalRead(uint8_t pin);
void pinMode(uint8_t pin, uint8_t mode);
void setupADC(void);
void enablePCI(uint8_t pin);
void scheduleAllAnalogReads(void);
int analogRead(uint8_t pin);
void stopReading(void);
void setUpValidPins(void);
void setUpAnalogDigitalPin(Pin_t button, uint8_t pin, uint16_t threshold);