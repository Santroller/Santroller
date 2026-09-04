// -----------------------------------------------------
// NOTE: THIS HEADER IS ALSO INCLUDED BY ASSEMBLER SO
//       SHOULD ONLY CONSIST OF PREPROCESSOR DIRECTIVES
// -----------------------------------------------------

#ifndef _BOARDS_PICO2_W_COMPAT_H
#define _BOARDS_PICO2_W_COMPAT_H

#include "boards/pico2_w.h"

#undef PICO_BOOT_STAGE2_CHOOSE_W25Q080
#define PICO_BOOT_STAGE2_CHOOSE_GENERIC_03H 1

#undef PICO_FLASH_SPI_CLKDIV
#define PICO_FLASH_SPI_CLKDIV 4

#undef PICO_RP2040_B0_SUPPORTED
#define PICO_RP2040_B0_SUPPORTED 1

#undef PICO_RP2040_B1_SUPPORTED
#define PICO_RP2040_B1_SUPPORTED 1

#endif