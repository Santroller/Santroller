#pragma once
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdint.h>
#include <stdbool.h>
#include "../../controller/controller.h"
#include "arduino_pins.h"

typedef struct {
  uint8_t mask;
  volatile uint8_t *port;
  uint16_t pmask;
  bool eq;
  uint8_t offset;
} pin_t;
typedef struct {
  uint8_t srb;
  uint8_t mux;
  uint8_t offset;
  uint8_t pin;
  bool inverted;
  volatile uint16_t value;
  // Does this analog pin map to a digital pin (aka drums)
  uint16_t threshold;
  bool hasDigital;
  pin_t digital;
} analog_info_t;
extern analog_info_t joyData[NUM_ANALOG_INPUTS];
extern int validAnalog;
void setUpPin(uint8_t pin);
int digitalRead(uint8_t pin);
void pinMode(uint8_t pin, uint8_t mode);
void enableADC(void);
void enablePCI(uint8_t pin);
unsigned long millis(void);
unsigned long micros(void);
void delay(unsigned long ms);
void resetADC(void);
void startADC(void);
int analogRead(uint8_t pin);
void stopReading(void);
void setUpValidPins(void);
void setUpAnalogDigitalPin(pin_t button, uint8_t pin, uint16_t threshold);