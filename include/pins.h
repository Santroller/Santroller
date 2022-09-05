#include <stdint.h>
#include "config.h"
void initPins(void);
void initDetectionDigital(void);
void initDetectionAnalog(void);
int detectAnalog();
void detectDigital(uint8_t* mask, uint8_t* pin);
void tickPins(void);
void stopReading(void);
uint8_t adc_raw(uint8_t pin);
// TODO: this should be done differently.
#if CONSOLE_TYPE == PC_XINPUT 
int16_t adc(uint8_t analogIndex, uint8_t offset, int16_t multiplier, uint8_t deadzone);
uint16_t adc_trigger(uint8_t analogIndex, uint8_t offset, int16_t multiplier, uint8_t deadzone);
#else
uint8_t adc(uint8_t analogIndex, uint8_t offset, int16_t multiplier, uint8_t deadzone);
uint8_t adc_trigger(uint8_t analogIndex, uint8_t offset, int16_t multiplier, uint8_t deadzone);
#endif