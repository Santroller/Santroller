#include "base.hpp"
#include "devices/usb.hpp"
class PS4Parser : public Parser {
   public:
    virtual void parse(uint8_t* report, uint8_t len, san_base_t* data);
    virtual void build(san_base_t* data);

   private:
    USBControllerType subType;
};