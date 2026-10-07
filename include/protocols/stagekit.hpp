#pragma once
#include "enums.pb.h"

// Subtypes whose "rumble" values are Rock Band stage kit / instrument LED commands
static inline bool subtype_supports_stagekit(SubType subtype)
{
    return subtype == StageKit || subtype == GuitarHeroDrums || subtype == RockBandDrums || subtype == GuitarHeroGuitar || subtype == RockBandGuitar || subtype == DjHeroTurntable;
}
