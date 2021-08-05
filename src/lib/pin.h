#pragma once
#include <stdint.h>

#include "config.h"
typedef struct Input_d Input_t;
typedef struct {
    int8_t multiplier;
    int8_t offset;
    int8_t deadzone;
    int8_t ADCtrigger;
    uint8_t channel;
    bool isADC;
    volatile int16_t currentValue;
    int16_t (*analogRead)(Input_t*);
} AnalogInput_t;

typedef struct Input_d {
    uint8_t binding;
    uint32_t lastMillis;
    uint8_t milliDeBounce;
    AnalogInput_t* axisInfo;  //For digital pins this will be NULL
    bool (*digitalRead)(Input_t*);
    bool pullup : 1;
    bool pulldown : 1;
#ifdef __AVR__
    volatile uint8_t* port;
    uint8_t mask;
#endif
} Input_t;
int16_t readAnalog(Input_t* pin);
bool readDigital(Input_t* pin);
void setDefaults(void);