#include "parsers/streamdeck.hpp"

#include "parsers/dpad.hpp"
#include "parsers/drums.hpp"
#include "parsers/gh5.hpp"
#include "parsers/ghwt.hpp"
#include "parsers/proguitar.hpp"
#include "parsers/prokeys.hpp"
#include "protocols/controller_reports.hpp"
#include "string.h"
void StreamDeckParser::parse(uint8_t *reportData, uint8_t len, san_base_t *data) {
    uint8_t offset = 1;
    switch (mType) {
        case STREAM_DECK_XL:
        case STREAM_DECK_MK2:
        case STREAM_DECK_NEO:
        case STREAM_DECK_V2:
        case STREAM_DECK_XLV2:
        case STREAM_DECK_PEDAL:
        case STREAM_DECK_PLUS:
            offset = 4;
            break;
    }
    for (uint8_t i = 0; i + offset < len && i < 16; i++) {
        if (reportData[i + offset]) {
            data->extra.streamDeckButtons &= ~(1 << i);
        } else {
            data->extra.streamDeckButtons |= (1 << i);
        }
    }
}