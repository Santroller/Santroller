#include "base.hpp"
#include "devices/usb.hpp"
#include "protocols/xbox_one.hpp"
#include <map>

class XboxOneChunk {
    uint8_t* buffer;
    int bufferLen;
};
class XboxOneParser : public Parser {
   public:
    virtual void parse(uint8_t* report, uint8_t len, san_base_t* data);
    virtual void build(san_base_t* data);

   private:
    USBDevice subType;
    std::map<uint8_t, uint8_t> previousReceiveSequence;
    std::map<uint8_t, uint8_t> previousSendSequence;
};