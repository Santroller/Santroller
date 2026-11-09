// -----------------------------------------------------
// NOTE: THIS HEADER IS ALSO INCLUDED BY ASSEMBLER SO
//       SHOULD ONLY CONSIST OF PREPROCESSOR DIRECTIVES
// -----------------------------------------------------

#ifndef _BOARDS_PICO2_W_COMPAT_H
#define _BOARDS_PICO2_W_COMPAT_H

// Lay flash out for 2MB like the RP2040 build, not the Pico 2's 4MB: plenty of RP2350 boards have a 2MB chip
// (eg W25Q16JV), and addresses past the end of the chip wrap around to its start, so a 4MB layout's download
// slot lands on the static firmware and the running app (and the swap on the bootloader). Set before
// boards/pico2_w.h, which only sets its 4MB if nothing has yet (CMake reads these lines in order too).
// Builds before 3526 kept the config at the end of 4MB, see adopt_config_from_old_layout.
pico_board_cmake_set(PICO_FLASH_SIZE_BYTES, (2 * 1024 * 1024))
#define PICO_FLASH_SIZE_BYTES (2 * 1024 * 1024)

#include "boards/pico2_w.h"

#define PICO_XOSC_STARTUP_DELAY_MULTIPLIER 64

#undef PICO_RP2040_B0_SUPPORTED
#define PICO_RP2040_B0_SUPPORTED 1

#undef PICO_RP2040_B1_SUPPORTED
#define PICO_RP2040_B1_SUPPORTED 1

// One build for the RP2350A (Pico 2 W) and RP2350B (eg Waveshare RP2350B-Plus-W): GPIO 30 - 47 only exist on
// the B, which is told apart at runtime (rp2350b() in devices/pins.hpp). The ADC's base pin and channel count
// are runtime too, as the SDK's ADC_BASE_PIN / NUM_ADC_CHANNELS are the B's here.
#undef PICO_RP2350A
#define PICO_RP2350A 0

// The wireless chip's pins are set at runtime (BluetoothStack::set_cyw43_pins), as boards wire it differently
#define CYW43_PIN_WL_DYNAMIC 1

#endif