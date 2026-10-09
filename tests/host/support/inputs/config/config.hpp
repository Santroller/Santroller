#pragma once
// Fake config/config.hpp: what the inputs, devices and triggers use from the config loader.
// reload() and the aux state updates are recorded instead of rewriting flash.
#include <stdint.h>
#include <vector>
#include <utility>
#include "mappings/mapping.hpp"
#include "devices/base.hpp"
#include "config.pb.h"
#include "enums.pb.h"

namespace fake_config
{
inline int reload_count = 0;
// (device id, state) for every cycle / toggle change saved to the aux config
inline std::vector<std::pair<uint32_t, uint32_t>> aux_cycles;
inline std::vector<std::pair<uint32_t, bool>> aux_toggles;
inline void reset()
{
    reload_count = 0;
    aux_cycles.clear();
    aux_toggles.clear();
}
}

void reload();
void update_aux_cycle(uint32_t id, uint32_t state);
void update_aux_toggle(uint32_t id, bool state);
