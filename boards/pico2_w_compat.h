// -----------------------------------------------------
// NOTE: THIS HEADER IS ALSO INCLUDED BY ASSEMBLER SO
//       SHOULD ONLY CONSIST OF PREPROCESSOR DIRECTIVES
// -----------------------------------------------------

#ifndef _BOARDS_PICO2_W_COMPAT_H
#define _BOARDS_PICO2_W_COMPAT_H

#include "boards/pico2_w.h"

#define PICO_XOSC_STARTUP_DELAY_MULTIPLIER 64

#undef PICO_RP2040_B0_SUPPORTED
#define PICO_RP2040_B0_SUPPORTED 1

#undef PICO_RP2040_B1_SUPPORTED
#define PICO_RP2040_B1_SUPPORTED 1

#endif