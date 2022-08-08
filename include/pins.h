#include <stdint.h>
#include "config.h"
void initPins(void);
void initDetectionDigital(void);
void initDetectionAnalog(void);
int detectAnalog();
void detectDigital(uint8_t* mask, uint8_t* pin);
void tickPins(void);
void stopReading(void);
#if CONSOLE_TYPE == PC_XINPUT 
int16_t adc(uint8_t analogIndex, int16_t offset, float multiplier, uint16_t deadzone);
uint16_t adc_trigger(uint8_t analogIndex, int16_t offset, float multiplier, uint16_t deadzone);
#else
uint8_t adc(uint8_t analogIndex, int16_t offset, float multiplier, uint16_t deadzone);
uint8_t adc_trigger(uint8_t analogIndex, int16_t offset, float multiplier, uint16_t deadzone);
#endif