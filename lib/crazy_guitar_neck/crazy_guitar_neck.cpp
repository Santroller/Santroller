#include "crazy_guitar_neck.hpp"
void CrazyGuitarNeck::tick() {
    uint8_t clone_buttons[4];
    static uint8_t clone_data_command[] = {0x53, 0x10, 0x00, 0x01};
    if (!reading) {
        interface.writeTo(CLONE_ADDR, clone_data_command, sizeof(clone_data_command), true, true);
        reading = true;
    } else {
        connected = interface.readFrom(CLONE_ADDR, clone_buttons, sizeof(clone_buttons), true);
        if (connected && clone_buttons[0] == CLONE_VALID_PACKET) {
            reading = false;
            green = clone_buttons[2] & 0x40;
            red = clone_buttons[2] & 0x01;
            yellow = clone_buttons[2] & 0x02;
            blue = clone_buttons[2] & 0x10;
            orange = clone_buttons[2] & 0x20;
            soloGreen = clone_buttons[1] & 0x08;
            soloRed = clone_buttons[1] & 0x04;
            soloYellow = clone_buttons[1] & 0x02;
            soloBlue = clone_buttons[1] & 0x01;
            soloOrange = clone_buttons[2] & 0x80;
        }
    }
};