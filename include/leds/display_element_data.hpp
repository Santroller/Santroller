#pragma once
#include <stdint.h>
#include <vector>
#include "config.pb.h"

// One frame of a display element's animation, with its image (empty when it uses the element's own)
struct DisplayFrameData
{
    proto_DisplayFrame frame;
    std::vector<uint8_t> image;
};

// What a display LED has that's too big for its config struct, decoded separately: its image and the
// frames of its animation
struct DisplayElementData
{
    // More frames than this are dropped, which keeps a config with lots of images from running out of RAM
    static constexpr size_t MAX_FRAMES = 16;
    std::vector<uint8_t> image;
    std::vector<DisplayFrameData> frames;
};
