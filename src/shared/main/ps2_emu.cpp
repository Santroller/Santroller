#include "ps2.h"

#include <stddef.h>

#include "Arduino.h"
#include "config.h"
#include "io.h"
#include "util.h"
#ifdef OUPUT_PS2
// TODO: https://github.com/matlo/GIMX-firmwares/blob/master/EMUPS2/example.c
// TODO: for AVR just set MISO as an output to turn on slave mode
// TODO: we already set the required defines for pico
#endif