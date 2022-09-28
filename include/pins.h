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
int16_t adc_xbox(uint8_t analogIndex, uint8_t offset, int16_t multiplier, uint8_t deadzone);
uint16_t adc_trigger_xbox(uint8_t analogIndex, uint8_t offset, int16_t multiplier, uint8_t deadzone);
uint8_t adc(uint8_t analogIndex, uint8_t offset, int16_t multiplier, uint8_t deadzone);
uint8_t adc_trigger(uint8_t analogIndex, uint8_t offset, int16_t multiplier, uint8_t deadzone);