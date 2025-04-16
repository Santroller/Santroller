#include "base.hpp"
#include "devices/usb.hpp"
#include "protocols/xbox_one.hpp"
#include <map>


class XboxOneParser : public Parser {
   public:
    XboxOneParser();
    void parse(uint8_t* report, uint8_t len, san_base_t* data);
    int build(san_base_t* data, uint8_t* output);

   private:
    uint8_t previousReceiveSequence[128];
    uint8_t previousSendSequence[128];
    XboxChunk chunk;
    int descriptorFailCount;
    Gip_Ack_t ack;
    bool needsAck = false;
};