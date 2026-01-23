#pragma once
#include <pb_decode.h>
#include <pb_encode.h>
#include <stdint.h>
#include <stdio.h>
#include <map>
#include <set>
#include <vector>
#include <memory>
#include "mappings/mapping.hpp"
#include "devices/base.hpp"

#include "config.pb.h"
#include "pico/stdlib.h"
#include "FlashPROM.h"
#include "CRC32.h"
#include "usb/device/device.hpp"
bool save(proto_Config *config);
bool load(proto_Config &config);
uint32_t copy_config(uint8_t *buffer, uint32_t start);
uint32_t copy_config_info(uint8_t *buffer);
bool write_config_info(const uint8_t *buffer, uint16_t bufsize);
bool write_config(const uint8_t *buffer, uint16_t bufsize, uint32_t start);
void update(bool full_poll);
void set_current_profile(uint32_t profile);
void first_load();
extern proto_SubType current_type;
extern ConsoleMode mode;
extern ConsoleMode newMode;
extern bool working;

extern std::vector<std::shared_ptr<Instance>> instances;
extern std::map<uint8_t, std::shared_ptr<UsbDevice>> usb_instances;
extern std::map<uint8_t, std::shared_ptr<UsbDevice>> usb_instances_by_epnum;
extern std::map<uint32_t, std::shared_ptr<Device>> devices;
extern std::vector<std::shared_ptr<Instance>> active_instances;
extern std::map<uint32_t, std::shared_ptr<Instance>> profiles;
extern std::set<uint32_t> active_profiles;
inline bool hid_based(void)
{
    return mode == ModeHid || mode == ModePs3 || mode == ModePs4 || mode == ModeWiiRb || mode == ModeSwitch;
}

inline bool ps4_based(void)
{
    // TODO: grab from the current preset too.
    return current_type == Gamepad || current_type == LiveGuitar;
}