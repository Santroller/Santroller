#pragma once
#include "../config/config.h"
// We need to look at commands sent by consoles so we can make sure that they dont conflict.
enum SerialCommands {
    COMMAND_REBOOT=0x30,
    COMMAND_GET_SIGNATURE,
    COMMAND_GET_CPU_FREQ,
    COMMAND_GET_BOARD,
    COMMAND_JUMP_BOOTLOADER,
    COMMAND_FIND_DIGITAL,
    COMMAND_FIND_ANALOG,
    COMMAND_FIND_CANCEL,
    COMMAND_GET_EXTENSION,
    COMMAND_AVRDUDE,
    COMMAND_READ_CONFIG,
    COMMAND_WRITE_CONFIG,
    COMMAND_WRITE_SUBTYPE,
    COMMAND_SET_LEDS
};
// TODO: rename leonardo to leo
// TODO: -multi prefix should just be -m
// TODO: if we are going to rename things, we could finally put the pro in front of pro micro
typedef struct {
    uint8_t ps3id[8];
    uint32_t cpu_freq;
    uint8_t board[7];
    uint8_t detectedPin;
    uint16_t extension;
    Configuration_t conf;
} data_t;