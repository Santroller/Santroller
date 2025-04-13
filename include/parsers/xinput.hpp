#include "base.hpp"
enum Subtype {
    XINPUT_GAMEPAD = 1,
    XINPUT_WHEEL = 2,
    XINPUT_ARCADE_STICK = 3,
    XINPUT_FLIGHT_STICK = 4,
    XINPUT_DANCE_PAD = 5,
    XINPUT_GUITAR = 6,
    XINPUT_GUITAR_ALTERNATE = 7,
    XINPUT_DRUMS = 8,
    XINPUT_STAGE_KIT = 9,
    XINPUT_GUITAR_BASS = 11,
    XINPUT_PRO_KEYS = 15,
    XINPUT_ARCADE_PAD = 19,
    XINPUT_TURNTABLE = 23,
    XINPUT_GUITAR_HERO_LIVE = 24,  // Not real, but nice for matching
    XINPUT_PRO_GUITAR = 25,
    XINPUT_GUITAR_WT = 26,  // Also not real, but nice for matching
    XINPUT_DISNEY_INFINITY_AND_LEGO_DIMENSIONS = 33,
    XINPUT_SKYLANDERS = 36,
    XINPUT_RB_DRUMS = 37,  // Also not real, but nice for matching
    XINPUT_GH_DRUMS = 38,  // Also not real, but nice for matching
};
class XInputParser : public Parser {
   public:
    virtual void parse(uint8_t* report, uint8_t len, san_base_t* data);
    virtual void build(san_base_t* data);

   private:
    Subtype subType;
};