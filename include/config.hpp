
#include <pb_decode.h>
#include <pb_encode.h>
#include <stdint.h>
#include <stdio.h>
#include <map>

#include "config.pb.h"
#include "pico/stdlib.h"
#include "FlashPROM.h"
#include "CRC32.h"
#include "state/base.hpp"

bool save(proto_Config *config);
bool load(proto_Config &config);
uint32_t copy_config(uint8_t* buffer,uint32_t start);
uint32_t copy_config_info(uint8_t* buffer);
bool write_config_info(const uint8_t* buffer, uint16_t bufsize);
bool write_config(const uint8_t* buffer, uint16_t bufsize, uint32_t start);
void update(bool full_poll);
void set_current_profile(uint32_t profile);