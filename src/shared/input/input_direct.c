#include "input_direct.h"
#include "../config/eeprom.h"
#include "../output/usb/Descriptors.h"
#include "../util.h"
#include "input_guitar.h"
#include "pins/pins.h"
#include <stdlib.h>
pin_t pinData[16];
int validPins = 0;
void direct_init() {
  uint8_t *pins = (uint8_t *)&config.pins;
  validPins = 0;
  setUpValidPins();
  if (config.main.input_type == DIRECT) {
    for (size_t i = 0; i < XBOX_BTN_COUNT; i++) {
      if (pins[i] != INVALID_PIN) {
        bool is_fret = (i >= XBOX_A || i == XBOX_LB);
        pin_t pin = {};
        pin.offset = i;
        pin.mask = digitalPinToBitMask(pins[i]);
        pin.port = portInputRegister(digitalPinToPort((pins[i])));
        pin.pmask = _BV(i);
        pin.eq = is_fret && config.main.fret_mode == LEDS_INLINE;
        if (is_drum() && is_fret) {
          // We should probably keep a list of drum specific buttons, instead of
          // using isfret
          // ADC is 10 bit, thereshold is specified as an 8 bit value, so shift
          // it
          setUpAnalogDigitalPin(pin, pins[i],
                                config.new_items.threshold_drums << 3);
        } else {
          pinMode(pins[i], pin.eq ? INPUT : INPUT_PULLUP);
          pinData[validPins++] = pin;
        }
      }
    }
  }
  startADC();
}
bool should_skip(uint8_t i) {
  #ifdef __AVR_ATmega328P__ 
    if (i == 13) return true;
  #endif
  // Skip sda + scl when using peripherials utilising I2C
  if ((config.main.tilt_type == MPU_6050) &&
      (i == PIN_WIRE_SDA || i == PIN_WIRE_SCL)) {
    return true;
  }
  // Skip SPI pins when using peripherials that utilise SPI
  if ((config.main.fret_mode == APA102) &&
      (i == PIN_SPI_MOSI || i == PIN_SPI_MISO || i == PIN_SPI_SCK ||
       i == PIN_SPI_SS)) {
    return true;
  }
  return false;
}
bool finding_digital = false;
bool finding_analog = false;
int last[NUM_ANALOG_INPUTS];
extern uint8_t detected_pin;
extern bool found_pin;
void find_digital(void) {
  stopReading();
  for (int i = 2; i < NUM_DIGITAL_PINS_NO_DUP; i++) {
    if (!should_skip(i)) { pinMode(i, INPUT_PULLUP); }
  }
  finding_digital = true;
}
void find_analog(void) {
  stopReading();
  for (int i = 0; i < NUM_ANALOG_INPUTS; i++) {
    pinMode(A0 + i, INPUT_PULLUP);
    last[i] = analogRead(i);
  }
  finding_analog = true;
}
void stop_searching(void) {
  if (finding_digital) {
    for (int i = 2; i < NUM_DIGITAL_PINS_NO_DUP; i++) {
      if (!should_skip(i)) { pinMode(i, INPUT); }
    }
  }
  finding_digital = finding_analog = false;
  direct_init();
}
void direct_tick(controller_t *controller) {
  if (finding_analog) {
    for (int i = 0; i < NUM_ANALOG_INPUTS; i++) {
      if (abs(analogRead(i) - last[i]) > 10) {
        finding_analog = false;
        direct_init();
        detected_pin = i + A0;
        found_pin = true;
        return;
      }
    }
    return;
  }
  if (finding_digital) {
    for (int i = 2; i < NUM_DIGITAL_PINS_NO_DUP; i++) {
      if (!should_skip(i)) {
        if (!digitalRead(i)) {
          stop_searching();
          detected_pin = i;
          found_pin = true;
          return;
        }
      }
    }
    return;
  }
  pin_t pin;
  for (uint8_t i = 0; i < validPins; i++) {
    pin = pinData[i];
    if ((*pin.port & pin.mask) == pin.eq) {
      controller->buttons |= pin.pmask;
      controller->all_axis[pin.offset] = MIDI_STANDARD_VELOCITY;
    } else {
      controller->all_axis[pin.offset] = 0;
    }
  }
  analog_info_t info;
  for (int8_t i = 0; i < validAnalog; i++) {
    info = joyData[i];
    if (info.hasDigital) {
      if (info.value > info.threshold) {
        controller->buttons |= info.digital.pmask;
      }
      controller->all_axis[XBOX_BTN_COUNT + info.offset] = info.value;
    } else if (info.offset >= 2) {
      ((controller_a_t *)controller)->sticks[info.offset - 2] = info.value;
    } else {
      ((controller_a_t *)controller)->triggers[info.offset] = info.value >> 8;
    }
  }

  resetADC();
}
