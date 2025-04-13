#include "devices/clone_neck.hpp"

#include "interfaces/core.hpp"

void CloneNeckDevice::tick(san_base_t* data) {
    uint8_t clone_buttons[4];
    static uint8_t clone_data_command[] = {0x53, 0x10, 0x00, 0x01};
    if (!clone_guitar_ready_timer) {
        clone_guitar_ready_timer = Core::millis();
    }
    if (Core::millis() - clone_guitar_ready_timer > 350) {
        clone_ready = true;
    }
    if (clone_ready) {
        if (!reading) {
            mInterface->writeTo(CLONE_ADDR, clone_data_command, sizeof(clone_data_command), true, true);
            reading = true;
        } else {
            mFound = mInterface->readFrom(CLONE_ADDR, clone_buttons, sizeof(clone_buttons), true);
            if (!mFound) {
                clone_guitar_ready_timer = Core::millis();
                clone_ready = false;
            }
            if (mFound && clone_buttons[0] == CLONE_VALID_PACKET) {
                reading = false;
                data->guitar.green = clone_buttons[2] & 0x40;
                data->guitar.red = clone_buttons[2] & 0x01;
                data->guitar.yellow = clone_buttons[2] & 0x02;
                data->guitar.blue = clone_buttons[2] & 0x10;
                data->guitar.orange = clone_buttons[2] & 0x20;
                data->guitar.soloGreen = clone_buttons[1] & 0x08;
                data->guitar.soloRed = clone_buttons[1] & 0x04;
                data->guitar.soloYellow = clone_buttons[1] & 0x02;
                data->guitar.soloBlue = clone_buttons[1] & 0x01;
                data->guitar.soloOrange = clone_buttons[2] & 0x80;
            }
        }
    }
}