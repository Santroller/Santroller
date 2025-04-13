#include "base.hpp"
#include "devices/usb.hpp"
enum StreamDeckType {
    NONE = 0,
    STREAM_DECK_OG = 1,
    STREAM_DECK_MINI = 2,
    STREAM_DECK_XL = 3,
    STREAM_DECK_V2 = 4,
    STREAM_DECK_MK2 = 5,
    STREAM_DECK_PLUS = 6,
    STREAM_DECK_PEDAL = 7,
    STREAM_DECK_XLV2 = 8,
    STREAM_DECK_MINIV2 = 9,
    STREAM_DECK_NEO = 10
};
class StreamDeckParser : public Parser {
   public:
    virtual void parse(uint8_t* report, uint8_t len, san_base_t* data);
    virtual void build(san_base_t* data);
    StreamDeckType mType = NONE;
};