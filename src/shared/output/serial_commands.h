#pragma once
#include "../config/config.h"
enum SerialCommands {
    COMMAND_REBOOT=0x30,
    COMMAND_JUMP_BOOTLOADER,
    COMMAND_GET_CPU_INFO,
    COMMAND_FIND_DIGITAL,
    COMMAND_FIND_ANALOG,
    COMMAND_GET_EXTENSION,
    COMMAND_GET_FOUND,
    COMMAND_GET_RF_CPU_INFO,
    COMMAND_WRITE_SUBTYPE,
    COMMAND_SET_LEDS,
    COMMAND_RESET,
    COMMAND_SET_SP,
    COMMAND_GET_VALUES,
    COMMAND_WRITE_CONFIG,
    COMMAND_READ_CONFIG,
    MAX
};
typedef struct {
    uint32_t cpu_freq;
    bool multi;
    char board[15];
    uint32_t rfID;
} cpu_info_t;

#define PACKET_SIZE 28