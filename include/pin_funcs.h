#include <stdint.h>
#include "config.h"
#include "pico_slave.h"
void initPins(void);
void initDetectionDigital(void);
void initDetectionAnalog(void);
int detectAnalog();
void detectDigital(uint8_t* mask, uint8_t* pin);
void stopReading(void);
uint16_t adc(uint8_t pin);
uint8_t digital_read(uint8_t port, uint8_t mask);
uint16_t adc_read(uint8_t pin, uint8_t mask);
uint16_t multiplexer_read(uint8_t pin, uint32_t mask, uint32_t bits);
void digital_write(uint8_t port, uint8_t mask, uint8_t activeMask);
void putWs2812(uint8_t r, uint8_t g, uint8_t b);
void putWs2812(uint8_t r, uint8_t g, uint8_t b, uint8_t w);