#include "profiles/profile.hpp"
#include "mappings/base_mapping.hpp"
#include "leds/leds.hpp"

Profile::~Profile()
{
    mappings.clear();
    triggers.clear();
    leds.clear();
    devices.clear();
}
