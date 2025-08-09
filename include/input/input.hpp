#pragma once
#include <stdint.h>
class Input
{
public:
    Input(){}
    virtual ~Input(){}
    virtual bool tickDigital() = 0;
    virtual uint16_t tickAnalog() = 0;
    virtual void setup() = 0;
};