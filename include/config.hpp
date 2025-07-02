
#include <pb_decode.h>
#include <pb_encode.h>
#include <stdint.h>
#include <stdio.h>
#include <map>

#include "config.pb.h"
#include "pico/stdlib.h"
#include "FlashPROM.h"
#include "CRC32.h"

bool save(proto_Config *config);
bool load(proto_Config &config);