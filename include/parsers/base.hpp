#include "state/base.hpp"
class Parser {
   public:
    virtual void parse(uint8_t* report, uint8_t len, san_base_t* data) = 0;
    virtual void build(san_base_t* data) = 0;
};