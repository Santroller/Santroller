#pragma once
#include <pb_decode.h>
#include <pb_encode.h>
#include <stdint.h>
#include <stdio.h>
#include <map>
#include <unordered_map>
#include <set>
#include <vector>
#include <memory>
#include "mappings/mapping.hpp"
#include "devices/base.hpp"

#include "config.pb.h"
#include "pico/stdlib.h"
#include "FlashPROM.h"
#include "CRC32.h"
#include "emulation/usb/device.hpp"
#include "devices/usb/host/host.hpp"
bool load_empty();
bool load();
uint32_t copy_config(uint8_t *buffer, uint32_t start);
uint32_t copy_config_info(uint8_t *buffer);
bool write_config_info(const uint8_t *buffer, uint16_t bufsize);
bool write_config(const uint8_t *buffer, uint16_t bufsize, uint32_t start);
void update();
void reload();
void update_aux_cycle(uint32_t id, uint32_t state);
void update_aux_toggle(uint32_t id, bool state);
