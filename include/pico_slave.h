#include "config.h"
#define SLAVE_ADDR 0x75
#define SLAVE_COMMAND_SET_PINMODE     0x01
#define SLAVE_COMMAND_SET_PIN         0x02
#define SLAVE_COMMAND_INIT_SPI        0x03
#define SLAVE_COMMAND_WRITE_SPI       0x04
#define SLAVE_COMMAND_GET_DIGITAL     0x05
#define SLAVE_COMMAND_GET_ANALOG      0x06
#define SLAVE_COMMAND_INIT_WT         0x07
#define SLAVE_COMMAND_GET_WT_RAW      0x08
#define SLAVE_COMMAND_GET_WT_GH5      0x09
#define SLAVE_COMMAND_INITIALISE      0x0A

#define PIN_MODE_INPUT_PULLUP   0
#define PIN_MODE_INPUT          1
#define PIN_MODE_INPUT_PULLDOWN 2
#define PIN_MODE_INPUT_BUSKEEP  3
#define PIN_MODE_ANALOG         4
#define PIN_MODE_OUTPUT         5
#define PIN_MODE_SPI            6

#ifdef SLAVE_TWI_PORT
void slavePinMode(uint8_t pin, uint8_t pinMode);

uint32_t slaveReadDigital(void);

uint16_t slaveReadAnalog(uint8_t pin);

void slaveWriteDigital(uint8_t pin, bool output);

uint16_t slaveReadMultiplexer(uint8_t pin, uint8_t channel, uint8_t s0, uint8_t s1, uint8_t s2);

uint16_t slaveReadMultiplexer(uint8_t pin, uint8_t channel, uint8_t s0, uint8_t s1, uint8_t s2, uint8_t s3);
uint8_t slaveReadWtRaw(void);
uint8_t slaveReadWtGh5(void);
bool slaveInit(void);
void slaveInitWt(void);
void slaveInitLED(uint8_t instance);
void slaveWriteLED(uint8_t data);
void slaveWriteAnalog(uint8_t pin, uint8_t val);
#endif