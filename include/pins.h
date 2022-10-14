#include <stdint.h>
#include "config.h"
void initPins(void);
void initDetectionDigital(void);
void initDetectionAnalog(void);
int detectAnalog();
void detectDigital(uint8_t* mask, uint8_t* pin);
void tickPins(void);
void stopReading(void);
uint16_t adc(uint8_t pin);
uint32_t digital_read(uint8_t port, uint8_t mask);
uint16_t adc_read(uint8_t pin, uint8_t mask);