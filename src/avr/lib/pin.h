#pragma once
#include <stdint.h>

#include "config.h"
#ifndef cbi
#  define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#  define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif
typedef struct {
    int8_t multiplier;
    int8_t offset;
    int8_t deadzone;
    int8_t ADCtrigger;
    uint8_t channel;
    bool isADC;
    volatile int16_t currentValue;
} AnalogPin_t;

typedef struct {
    uint8_t binding;
    volatile uint8_t* port;
    uint8_t mask;
    uint32_t lastMillis;
    uint8_t milliDeBounce;
    bool pullup;
    AnalogPin_t* axisInfo;  //For digital pins this will be NULL
} Pin_t;
int16_t readAnalog(Pin_t* pin);
bool readDigital(Pin_t* pin);
void setDefaults(void);