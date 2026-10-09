#pragma once
// The firmware's TinyUSB config, without the Pico SDK OS layer (no semaphores or mutexes on the host)
#include_next "tusb_config.h"
#undef CFG_TUSB_OS
#define CFG_TUSB_OS OPT_OS_NONE
