// -----------------------------------------------------
// NOTE: THIS HEADER IS ALSO INCLUDED BY ASSEMBLER SO
//       SHOULD ONLY CONSIST OF PREPROCESSOR DIRECTIVES
// -----------------------------------------------------

#ifndef _BOARDS_PICO_W_COMPAT_H
#define _BOARDS_PICO_W_COMPAT_H

#include "boards/pico_w.h"

#define PICO_XOSC_STARTUP_DELAY_MULTIPLIER 64

#undef PICO_RP2040_B0_SUPPORTED
#define PICO_RP2040_B0_SUPPORTED 1

#undef PICO_RP2040_B1_SUPPORTED
#define PICO_RP2040_B1_SUPPORTED 1

// The wireless chip's pins are set at runtime (BluetoothStack::set_cyw43_pins), as boards wire it differently
#define CYW43_PIN_WL_DYNAMIC 1

#endif