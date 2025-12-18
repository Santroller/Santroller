#pragma once
#include <pb_decode.h>
#include <pb_encode.h>
#include <stdint.h>
#include <stdio.h>
#include <map>
#include <vector>
#include <memory>
#include "mappings/mapping.hpp"
#include "devices/base.hpp"

#include "config.pb.h"
#include "pico/stdlib.h"
#include "FlashPROM.h"
#include "CRC32.h"
#include "state/base.hpp"

bool save(proto_Config *config);
bool load(proto_Config &config);
uint32_t copy_config(uint8_t *buffer, uint32_t start);
uint32_t copy_config_info(uint8_t *buffer);
bool write_config_info(const uint8_t *buffer, uint16_t bufsize);
bool write_config(const uint8_t *buffer, uint16_t bufsize, uint32_t start);
void update(bool full_poll);
void set_current_profile(uint32_t profile);
extern proto_SubType current_type;
extern ConsoleMode mode;
extern ConsoleMode newMode;
extern bool working;

extern std::vector<std::unique_ptr<Mapping>> mappings;
extern std::map<uint32_t, std::shared_ptr<Device>> devices;
extern std::vector<std::unique_ptr<ActivationTrigger>> triggers;
inline bool hid_based(void)
{
    return mode == ModeHid || mode == ModePs3 || mode == ModePs4 || mode == ModeWiiRb || mode == ModeSwitch;
}

inline bool ps4_based(void)
{
    // TODO: grab from the current preset too.
    return current_type == Gamepad || current_type == LiveGuitar;
}

// Instead of having a global mode and mappings, we have a mode and mappings per instance.
// that would let us handle things like multiple adapters at once.
// TODO: how do we express the ability to have multiple profiles mapped at once?
class Instance
{
public:
    uint8_t epin;
    uint8_t epout;
    SubType subtype;
    ConsoleMode mode;
    uint8_t profile_id;
    std::vector<std::unique_ptr<Mapping>> mappings;
};