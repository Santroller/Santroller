#include "base.hpp"
#include "devices/usb.hpp"
enum DancepadType {
    LTEK = 23,
    STEPMANIAX = 24,
    LTEK_ID = 25
};
class DancepadParser : public Parser {
   public:
    virtual void parse(uint8_t* report, uint8_t len, san_base_t* data);
    virtual void build(san_base_t* data);

   private:
    DancepadType mType;
};