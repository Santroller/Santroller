#include "base.hpp"
#include "devices/usb.hpp"
// TODO: for this, we should actually just outright parse the packets fully and do this right
class XboxOneParser : public Parser {
   public:
    virtual void parse(uint8_t* report, uint8_t len, san_base_t* data);
    virtual void build(san_base_t* data);

   private:
    USBDevice subType;
};