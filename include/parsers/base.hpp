#pragma once
#include "state/base.hpp"
class Parser {
   public:
    virtual void parse(uint8_t* report, uint8_t len, san_base_t* data) = 0;
    virtual int build(san_base_t* data, uint8_t* output) = 0;
};