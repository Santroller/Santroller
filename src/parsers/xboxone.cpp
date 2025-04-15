// TODO: for this, we should actually just outright parse the packets fully and do this right
// could potentially even look at matching via guid instead of vid and pid
// but we put all the xb1 stuff here including auth.
#include "parsers/xboxone.hpp"

#include <stdint.h>

void XboxOneParser::parse(uint8_t* report, uint8_t len, san_base_t* data) {
    // Essentially, we would parse data from the controller here and store it locally to the parser as state
    // only real thing we have to figure out is a decent way to handle chunked messages
}

void XboxOneParser::build(san_base_t* data) {
    // And then here we return controller inputs when auth is done, but otherwise before that we can return data for auth and descriptors and such
}