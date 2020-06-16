#pragma once
// We need to look at commands sent by consoles so we can make sure that they dont conflict.
enum SerialCommands {
    COMMAND_GET_HID_REPORT,
    // PS3 controllers have a hid feature report that is required for identifying the controller.
    COMMAND_GET_PS3_ID = 3,
    COMMAND_REBOOT,
    COMMAND_GET_SIGNATURE,
    COMMAND_GET_CPU_FREQ,
    COMMAND_GET_BOARD,
    COMMAND_JUMP_BOOTLOADER_UNO,
    COMMAND_JUMP_BOOTLOADER,
    COMMAND_FIND_DIGITAL,
    COMMAND_FIND_ANALOG,
    COMMAND_FIND_STOP,
    COMMAND_GET_EXTENSION,
    COMMAND_AVRDUDE,
    COMMAND_CONFIG,
};