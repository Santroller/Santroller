#include "base.hpp"
typedef enum {
    RNT_TYPE_NONE_NEW = 100,
    RNT_TYPE_CLASSIC = 101,
    RNT_TYPE_NUNCHUK = 112,
    RNT_TYPE_CLASSIC_PRO = 113,
    RNT_TYPE_WIIMOTE_TAIKO = 114,
    RNT_TYPE_PSX_DIGITAL = 119,
    RNT_TYPE_PSX_ANALOG = 120,
    RNT_TYPE_PSX_NEGCON = 121,
    RNT_TYPE_PSX_MOUSE = 122,
    RNT_TYPE_WII_GUITAR = 127,
    RNT_TYPE_UDRAW_TABLET = 128,
    RNT_TYPE_WII_DRUM = 130
} RaphnetType_t;
class RaphnetParser : public Parser {
   public:
    virtual void parse(uint8_t* report, uint8_t len, san_base_t* data);
    virtual void build(san_base_t* data);

    bool hiRes = false;
    bool hasTapBar = false;
    RaphnetType_t mType = RNT_TYPE_NONE_NEW;
};