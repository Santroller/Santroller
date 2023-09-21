#include "config.h"
#define SLAVE_ADDR 0x75
#define SLAVE_COMMAND_SET_PINMODE         0x01
#define SLAVE_COMMAND_SET_PIN             0x02
#define SLAVE_COMMAND_INIT_SPI            0x03
#define SLAVE_COMMAND_WRITE_SPI           0x04
#define SLAVE_COMMAND_GET_DIGITAL         0x05
#define SLAVE_COMMAND_GET_DIGITAL_PIN_2   0x08
#define SLAVE_COMMAND_INIT_WT             0x0A
#define SLAVE_COMMAND_GET_WT              0x0B
#define SLAVE_COMMAND_GET_WT_RAW          0x0C
#define SLAVE_COMMAND_INITIALISE          0x0D

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


uint8_t slaveReadDigital(uint8_t port, uint8_t mask);

void slaveWriteDigital(uint8_t pin, bool output);
uint8_t slaveReadWt(void);
uint8_t slaveReadWtRaw(uint8_t* dest);
bool slaveInit(void);
void slaveInitWt(void);
void slaveSetWtCounter(uint16_t counter);
void slaveInitLED(uint8_t instance);
void slaveWriteLED(uint8_t data);
void slaveWriteAnalog(uint8_t pin, uint8_t val);
#endif