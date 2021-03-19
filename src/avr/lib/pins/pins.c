#include "pins/pins.h"
#include "controller/guitar_includes.h"
#include "eeprom/eeprom.h"
#include "stddef.h"
#include "util/util.h"
#include <avr/interrupt.h>
// On the ATmega1280, the addresses of some of the port registers are
// greater than 255, so we can't store them in uint8_t's.
extern const uint16_t PROGMEM port_to_mode_PGM[];
extern const uint16_t PROGMEM port_to_input_PGM[];
extern const uint16_t PROGMEM port_to_output_PGM[];

extern const uint8_t PROGMEM digital_pin_to_port_PGM[];
// extern const uint8_t PROGMEM digital_pin_to_bit_PGM[];
extern const uint8_t PROGMEM digital_pin_to_bit_mask_PGM[];
extern const uint8_t PROGMEM digital_pin_to_timer_PGM[];
extern const uint8_t PROGMEM analog_pin_to_channel_PGM[];
#define digitalPinToPort(P) (pgm_read_byte(digital_pin_to_port_PGM + (P)))
#define digitalPinToBitMask(P)                                                 \
  (pgm_read_byte(digital_pin_to_bit_mask_PGM + (P)))
#define digitalPinToTimer(P) (pgm_read_byte(digital_pin_to_timer_PGM + (P)))
#define analogInPinToBit(P) (P)
#define portOutputRegister(P)                                                  \
  ((volatile uint8_t *)(pgm_read_word(port_to_output_PGM + (P))))
#define portInputRegister(P)                                                   \
  ((volatile uint8_t *)(pgm_read_word(port_to_input_PGM + (P))))
#define portModeRegister(P)                                                    \
  ((volatile uint8_t *)(pgm_read_word(port_to_mode_PGM + (P))))
int validAnalog = 0;
int currentAnalog = 0;
bool first = true;
Pin_t setUpDigital(uint8_t pinNum, uint8_t offset, bool inverted) {
  Pin_t pin = {};
  uint8_t port = digitalPinToPort(pinNum);
  pin.offset = offset;
  pin.mask = digitalPinToBitMask(pinNum);
  pin.port = portInputRegister(port);
  pin.outPort = portOutputRegister(port);
  pin.pmask = _BV(offset);
  pin.eq = inverted;
  pin.milliDeBounce = 5;
  pin.sioFunc = true;
  return pin;
}
bool digitalReadPin(Pin_t pin) {
  return ((*pin.port & pin.mask) != 0) == pin.eq;
}

void digitalWritePin(Pin_t pin, bool value) {
  if (value == 0) {
    *pin.outPort &= ~pin.mask;
  } else {
    *pin.outPort |= pin.mask;
  }
}
void digitalWrite(uint8_t pin, uint8_t val) {
  uint8_t bit = digitalPinToBitMask(pin);
  uint8_t port = digitalPinToPort(pin);
  volatile uint8_t *out;

  if (port == NOT_A_PIN) return;

  out = portOutputRegister(port);

  uint8_t oldSREG = SREG;
  cli();

  if (val == 0) {
    *out &= ~bit;
  } else {
    *out |= bit;
  }

  SREG = oldSREG;
}

int digitalRead(uint8_t pin) {
  uint8_t bit = digitalPinToBitMask(pin);
  uint8_t port = digitalPinToPort(pin);

  if (port == NOT_A_PIN) return 0;

  if (*portInputRegister(port) & bit) return 1;
  return 0;
}

void setUpAnalogPin(uint8_t offset) {
  AnalogInfo_t ret = {0};
  ret.offset = offset;
  AnalogPin_t apin = ((PinsCombined_t *)&config.pins)->axis[offset];
  uint8_t pin = apin.pin;
  if (pin == INVALID_PIN) { return; }
  if (ret.offset == 5 && isGuitar(config.main.subType) &&
      config.main.tiltType != ANALOGUE) {
    return;
  }
  ret.hasDigital = false;
  ret.inverted = apin.inverted;
#if defined(analogPinToChannel)
#  if defined(__AVR_ATmega32U4__)
  if (pin >= 18) pin -= 18; // allow for channel or pin numbers
#  endif
  pin = analogPinToChannel(pin);
#elif defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
  if (pin >= 54) pin -= 54; // allow for channel or pin numbers
#elif defined(__AVR_ATmega32U4__)
  if (pin >= 18) pin -= 18; // allow for channel or pin numbers
#elif defined(__AVR_ATmega1284__) || defined(__AVR_ATmega1284P__) ||           \
    defined(__AVR_ATmega644__) || defined(__AVR_ATmega644A__) ||               \
    defined(__AVR_ATmega644P__) || defined(__AVR_ATmega644PA__)
  if (pin >= 24) pin -= 24; // allow for channel or pin numbers
#else
  if (pin >= 14) pin -= 14; // allow for channel or pin numbers
#endif

  pinMode(PIN_A0 + pin, INPUT);
#if defined(ADCSRB) && defined(MUX5)
  // the MUX5 bit of ADCSRB selects whether we're reading from channels
  // 0 to 7 (MUX5 low) or 8 to 15 (MUX5 high).
  ret.srb = (ADCSRB & ~(1 << MUX5)) | (((pin >> 3) & 0x01) << MUX5);
#endif

  // set the analog reference (high two bits of ADMUX) and select the
  // channel (low 4 bits).  this also sets ADLAR (left-adjust result)
  // to 0 (the default).

  ret.mux = (1 << 6) | (pin & 0x07);
  joyData[validAnalog++] = ret;
}
void setUpAnalogDigitalPin(Pin_t button, uint8_t pin, uint16_t threshold) {
  AnalogInfo_t ret = {0};
  ret.offset = pin;
  ret.hasDigital = true;
  ret.digital = button;
  ret.threshold = threshold;
#if defined(analogPinToChannel)
#  if defined(__AVR_ATmega32U4__)
  if (pin >= 18) pin -= 18; // allow for channel or pin numbers
#  endif
  pin = analogPinToChannel(pin);
#elif defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
  if (pin >= 54) pin -= 54; // allow for channel or pin numbers
#elif defined(__AVR_ATmega32U4__)
  if (pin >= 18) pin -= 18; // allow for channel or pin numbers
#elif defined(__AVR_ATmega1284__) || defined(__AVR_ATmega1284P__) ||           \
    defined(__AVR_ATmega644__) || defined(__AVR_ATmega644A__) ||               \
    defined(__AVR_ATmega644P__) || defined(__AVR_ATmega644PA__)
  if (pin >= 24) pin -= 24; // allow for channel or pin numbers
#else
  if (pin >= 14) pin -= 14; // allow for channel or pin numbers
#endif

  pinMode(PIN_A0 + pin, INPUT);
#if defined(ADCSRB) && defined(MUX5)
  // the MUX5 bit of ADCSRB selects whether we're reading from channels
  // 0 to 7 (MUX5 low) or 8 to 15 (MUX5 high).
  ret.srb = (ADCSRB & ~(1 << MUX5)) | (((pin >> 3) & 0x01) << MUX5);
#endif

  // set the analog reference (high two bits of ADMUX) and select the
  // channel (low 4 bits).  this also sets ADLAR (left-adjust result)
  // to 0 (the default).

  ret.mux = (1 << 6) | (pin & 0x07);
  joyData[validAnalog++] = ret;
}
void tickAnalog(void) {
  if (validAnalog == 0) return;
  if (!first) {
    if (bit_is_set(ADCSRA, ADSC)) return;
    uint8_t low, high;
    low = ADCL;
    high = ADCH;
    int16_t data = (high << 8) | low;
    AnalogInfo_t *info = &joyData[currentAnalog];
    if (!info->hasDigital) {
      data = data - 512;
      if (info->inverted) data = -data;
      data = data * 64;
    }
    info->value = data;
    currentAnalog++;
    if (currentAnalog == validAnalog) { currentAnalog = 0; }
  }
  first = false;
  AnalogInfo_t info = joyData[currentAnalog];
#if defined(ADCSRB) && defined(MUX5)
  ADCSRB = info.srb;
#endif
  ADMUX = info.mux;
  sbi(ADCSRA, ADSC);
}

uint16_t analogRead(uint8_t pin) {
  uint8_t low, high;

#if defined(analogPinToChannel)
#  if defined(__AVR_ATmega32U4__)
  if (pin >= 18) pin -= 18; // allow for channel or pin numbers
#  endif
  pin = analogPinToChannel(pin);
#elif defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
  if (pin >= 54) pin -= 54; // allow for channel or pin numbers
#elif defined(__AVR_ATmega32U4__)
  if (pin >= 18) pin -= 18; // allow for channel or pin numbers
#elif defined(__AVR_ATmega1284__) || defined(__AVR_ATmega1284P__) ||           \
    defined(__AVR_ATmega644__) || defined(__AVR_ATmega644A__) ||               \
    defined(__AVR_ATmega644P__) || defined(__AVR_ATmega644PA__)
  if (pin >= 24) pin -= 24; // allow for channel or pin numbers
#else
  if (pin >= 14) pin -= 14; // allow for channel or pin numbers
#endif

#if defined(ADCSRB) && defined(MUX5)
  // the MUX5 bit of ADCSRB selects whether we're reading from channels
  // 0 to 7 (MUX5 low) or 8 to 15 (MUX5 high).
  ADCSRB = (ADCSRB & ~(1 << MUX5)) | (((pin >> 3) & 0x01) << MUX5);
#endif

  // set the analog reference (high two bits of ADMUX) and select the
  // channel (low 4 bits).  this also sets ADLAR (left-adjust result)
  // to 0 (the default).
#if defined(ADMUX)
#  if defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) ||                \
      defined(__AVR_ATtiny85__)
  ADMUX = (1 << 4) | (pin & 0x07);
#  else
  ADMUX = (1 << 6) | (pin & 0x07);
#  endif
#endif

  // without a delay, we seem to read from the wrong channel
  // delay(1);

#if defined(ADCSRA) && defined(ADCL)
  // start the conversion
  sbi(ADCSRA, ADSC);

  // ADSC is cleared when the conversion finishes
  while (bit_is_set(ADCSRA, ADSC))
    ;

  // we have to read ADCL first; doing so locks both ADCL
  // and ADCH until ADCH is read.  reading ADCL second would
  // cause the results of each conversion to be discarded,
  // as ADCL and ADCH would be locked when it completed.
  low = ADCL;
  high = ADCH;
#else
  // we dont have an ADC, return 0
  low = 0;
  high = 0;
#endif

  // combine the two bytes
  return (high << 8) | low;
}
void stopReading(void) {
  while (bit_is_set(ADCSRA, ADSC))
    ;
  first = true;
}
void pinMode(uint8_t pin, uint8_t mode) {
  uint8_t bit = digitalPinToBitMask(pin);
  uint8_t port = digitalPinToPort(pin);
  volatile uint8_t *reg, *out;

  if (port == NOT_A_PIN) return;

  reg = portModeRegister(port);
  out = portOutputRegister(port);

  if (mode == INPUT) {
    uint8_t oldSREG = SREG;
    cli();
    *reg &= ~bit;
    *out &= ~bit;
    SREG = oldSREG;
  } else if (mode == INPUT_PULLUP) {
    uint8_t oldSREG = SREG;
    cli();
    *reg &= ~bit;
    *out |= bit;
    SREG = oldSREG;
  } else {
    uint8_t oldSREG = SREG;
    cli();
    *reg |= bit;
    SREG = oldSREG;
  }
  sei();
}

void setupADC(void) {
#if defined(ADCSRA)
// set a2d prescaler so we are inside the desired 50-200 KHz range.
#  if F_CPU >= 16000000 // 16 MHz / 128 = 125 KHz
  sbi(ADCSRA, ADPS2);
  sbi(ADCSRA, ADPS1);
  sbi(ADCSRA, ADPS0);
#  elif F_CPU >= 8000000 // 8 MHz / 64 = 125 KHz
  sbi(ADCSRA, ADPS2);
  sbi(ADCSRA, ADPS1);
  cbi(ADCSRA, ADPS0);
#  elif F_CPU >= 4000000 // 4 MHz / 32 = 125 KHz
  sbi(ADCSRA, ADPS2);
  cbi(ADCSRA, ADPS1);
  sbi(ADCSRA, ADPS0);
#  elif F_CPU >= 2000000 // 2 MHz / 16 = 125 KHz
  sbi(ADCSRA, ADPS2);
  cbi(ADCSRA, ADPS1);
  cbi(ADCSRA, ADPS0);
#  elif F_CPU >= 1000000 // 1 MHz / 8 = 125 KHz
  cbi(ADCSRA, ADPS2);
  sbi(ADCSRA, ADPS1);
  sbi(ADCSRA, ADPS0);
#  else // 128 kHz / 2 = 64 KHz -> This is the closest you can get, the
        // prescaler is 2
  cbi(ADCSRA, ADPS2);
  cbi(ADCSRA, ADPS1);
  sbi(ADCSRA, ADPS0);
#  endif
  // enable a2d conversions
  sbi(ADCSRA, ADEN);
#endif
  // Enable adc interrupts
  // sbi(ADCSRA, ADIE);
}

void setUpValidPins(void) {
  stopReading();
  validAnalog = 0;
  currentAnalog = 0;
  for (int i = 0; i < 6; i++) { setUpAnalogPin(i); }
}