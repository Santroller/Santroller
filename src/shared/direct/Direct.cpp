#include "./Direct.h"
int digitalRead(uint8_t pin) {
  uint8_t bit = digitalPinToBitMask(pin);
  uint8_t port = digitalPinToPort(pin);

  if (port == NOT_A_PIN)
    return 0;

  if (*portInputRegister(port) & bit)
    return 1;
  return 0;
}
int analogRead(uint8_t pin) {
  uint8_t low, high;

#if defined(analogPinToChannel)
#if defined(__AVR_ATmega32U4__)
  if (pin >= 18)
    pin -= 18; // allow for channel or pin numbers
#endif
  pin = analogPinToChannel(pin);
#elif defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
  if (pin >= 54)
    pin -= 54; // allow for channel or pin numbers
#elif defined(__AVR_ATmega32U4__)
  if (pin >= 18)
    pin -= 18; // allow for channel or pin numbers
#elif defined(__AVR_ATmega1284__) || defined(__AVR_ATmega1284P__) ||           \
    defined(__AVR_ATmega644__) || defined(__AVR_ATmega644A__) ||               \
    defined(__AVR_ATmega644P__) || defined(__AVR_ATmega644PA__)
  if (pin >= 24)
    pin -= 24; // allow for channel or pin numbers
#else
  if (pin >= 14)
    pin -= 14; // allow for channel or pin numbers
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
#if defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) ||                  \
    defined(__AVR_ATtiny85__)
  ADMUX = (analog_reference << 4) | (pin & 0x07);
#else
  ADMUX = (0 << 6) | (pin & 0x07);
#endif
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
void pinMode(uint8_t pin, uint8_t mode) {
  uint8_t bit = digitalPinToBitMask(pin);
  uint8_t port = digitalPinToPort(pin);
  volatile uint8_t *reg, *out;

  if (port == NOT_A_PIN)
    return;

  // JWS: can I let the optimizer do this?
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
}
void Direct::read_controller(Controller *controller) {
#if DEVICE_TYPE == DIRECT
  bit_write(!digitalRead(PIN_GREEN), controller->buttons, GREEN);
  bit_write(!digitalRead(PIN_RED), controller->buttons, RED);
  bit_write(!digitalRead(PIN_YELLOW), controller->buttons, YELLOW);
  bit_write(!digitalRead(PIN_BLUE), controller->buttons, BLUE);
  bit_write(!digitalRead(PIN_ORANGE), controller->buttons, ORANGE);
  bit_write(!digitalRead(PIN_START), controller->buttons, START);
  bit_write(!digitalRead(PIN_SELECT), controller->buttons, SELECT);
  bit_write(!digitalRead(PIN_LEFT), controller->buttons, LEFT);
  bit_write(!digitalRead(PIN_RIGHT), controller->buttons, RIGHT);
  bit_write(!digitalRead(PIN_UP), controller->buttons, UP);
  bit_write(!digitalRead(PIN_DOWN), controller->buttons, DOWN);
  controller->r_x = 1024-analogRead(PIN_WHAMMY);
#endif
}

void Direct::init() {
#if DEVICE_TYPE == DIRECT
  pinMode(PIN_GREEN, INPUT_PULLUP);
  pinMode(PIN_RED, INPUT_PULLUP);
  pinMode(PIN_YELLOW, INPUT_PULLUP);
  pinMode(PIN_BLUE, INPUT_PULLUP);
  pinMode(PIN_ORANGE, INPUT_PULLUP);
  pinMode(PIN_START, INPUT_PULLUP);
  pinMode(PIN_SELECT, INPUT_PULLUP);
  pinMode(PIN_LEFT, INPUT_PULLUP);
  pinMode(PIN_RIGHT, INPUT_PULLUP);
  pinMode(PIN_UP, INPUT_PULLUP);
  pinMode(PIN_DOWN, INPUT_PULLUP);
  pinMode(PIN_WHAMMY, INPUT_PULLUP);
#endif
}