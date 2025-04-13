#include "parsers/ps2.hpp"
void PS2Parser::parse(uint8_t* ps2Data, uint8_t len, san_base_t* data) {
    switch (mType) {
        // buttons apply to digital, ds1 and ds2
        case PSX_DIGITAL:
        case PSX_FLIGHTSTICK:
        case PSX_JOGCON:
        case PSX_DUALSHOCK_1_CONTROLLER:
        case PSX_DUALSHOCK_2_CONTROLLER:
            data->gamepad.a = (~ps2Data[4]) & (1 << 6);
            data->gamepad.b = (~ps2Data[4]) & (1 << 5);
            data->gamepad.x = (~ps2Data[4]) & (1 << 7);
            data->gamepad.y = (~ps2Data[4]) & (1 << 4);
            data->gamepad.leftShoulder = (~ps2Data[4]) & (1 << 2);
            data->gamepad.rightShoulder = (~ps2Data[4]) & (1 << 3);
            data->gamepad.l2 = (~ps2Data[4]) & (1 << 0);
            data->gamepad.r2 = (~ps2Data[4]) & (1 << 1);
            data->gamepad.leftThumbClick = (~ps2Data[3]) & (1 << 1);
            data->gamepad.rightThumbClick = (~ps2Data[3]) & (1 << 2);
            data->gamepad.dpadUp = (~ps2Data[3]) & (1 << 4);
            data->gamepad.dpadDown = (~ps2Data[3]) & (1 << 6);
            data->gamepad.dpadLeft = (~ps2Data[3]) & (1 << 7);
            data->gamepad.dpadRight = (~ps2Data[3]) & (1 << 5);
            data->gamepad.start = (~ps2Data[3]) & (1 << 3);
            data->gamepad.back = (~ps2Data[3]) & (1 << 0);
            break;
        case PSX_GUITAR_HERO_CONTROLLER:
            data->gamepad.start = (~ps2Data[3]) & (1 << 3);
            data->gamepad.back = (~ps2Data[3]) & (1 << 0);
            data->guitar.green = (~ps2Data[4]) & (1 << 1);
            data->guitar.red = (~ps2Data[4]) & (1 << 5);
            data->guitar.yellow = (~ps2Data[4]) & (1 << 4);
            data->guitar.blue = (~ps2Data[4]) & (1 << 6);
            data->guitar.orange = (~ps2Data[4]) & (1 << 7);
            data->guitar.whammy = -(ps2Data[8] - 0x80) << 9;
            data->guitar.strumUp = (~ps2Data[3]) & (1 << 4);
            data->guitar.strumDown = (~ps2Data[3]) & (1 << 6);
            data->guitar.tilt = (~ps2Data[4]) & (1 << 0);
            // Tap bar -> solo frets
            if (ps2Data[7] <= 0x2F) {
                data->guitar.soloGreen = true;
                data->guitar.soloRed = false;
                data->guitar.soloYellow = false;
                data->guitar.soloBlue = false;
                data->guitar.soloOrange = false;
            } else if (ps2Data[7] <= 0x3F) {
                data->guitar.soloGreen = true;
                data->guitar.soloRed = true;
                data->guitar.soloYellow = false;
                data->guitar.soloBlue = false;
                data->guitar.soloOrange = false;
            } else if (ps2Data[7] <= 0x5F) {
                data->guitar.soloGreen = false;
                data->guitar.soloRed = true;
                data->guitar.soloYellow = false;
                data->guitar.soloBlue = false;
                data->guitar.soloOrange = false;
            } else if (ps2Data[7] <= 0x6F) {
                data->guitar.soloGreen = false;
                data->guitar.soloRed = true;
                data->guitar.soloYellow = true;
                data->guitar.soloBlue = false;
                data->guitar.soloOrange = false;
            } else if (ps2Data[7] <= 0x8F) {
                data->guitar.soloGreen = false;
                data->guitar.soloRed = false;
                data->guitar.soloYellow = false;
                data->guitar.soloBlue = false;
                data->guitar.soloOrange = false;
            } else if (ps2Data[7] <= 0x9F) {
                data->guitar.soloGreen = false;
                data->guitar.soloRed = false;
                data->guitar.soloYellow = true;
                data->guitar.soloBlue = false;
                data->guitar.soloOrange = false;
            } else if (ps2Data[7] <= 0xBF) {
                data->guitar.soloGreen = false;
                data->guitar.soloRed = false;
                data->guitar.soloYellow = true;
                data->guitar.soloBlue = true;
                data->guitar.soloOrange = false;
            } else if (ps2Data[7] <= 0xCF) {
                data->guitar.soloGreen = false;
                data->guitar.soloRed = false;
                data->guitar.soloYellow = false;
                data->guitar.soloBlue = true;
                data->guitar.soloOrange = false;
            } else if (ps2Data[7] <= 0xEF) {
                data->guitar.soloGreen = false;
                data->guitar.soloRed = false;
                data->guitar.soloYellow = false;
                data->guitar.soloBlue = true;
                data->guitar.soloOrange = true;
            } else {
                data->guitar.soloGreen = false;
                data->guitar.soloRed = false;
                data->guitar.soloYellow = false;
                data->guitar.soloBlue = false;
                data->guitar.soloOrange = true;
            }
            // Guitars put dpad on an axis
            data->gamepad.dpadLeft = (((ps2Data[5] >> 6) == 1) && ps2Data[5] != 127);
            data->gamepad.dpadRight = (((ps2Data[5] >> 6) == 3) && ps2Data[5] != 255);
            data->gamepad.dpadUp = ((ps2Data[5] >> 6) == 0);
            data->gamepad.dpadDown = (((ps2Data[5] >> 6) == 2) && ps2Data[5] != 128);
            break;
        case PSX_NEGCON:
            // Where necessary we create analog data out of the digital bits
            data->wheel.wheel = (ps2Data[5] - 128) << 8;
            data->gamepad.start = (~ps2Data[3]) & (1 << 3);
            data->gamepad.a = ps2Data[6];                // I -> Cross (A)
            data->gamepad.b = (~ps2Data[4]) & (1 << 5);  // A -> Circle (B)
            data->gamepad.x = ps2Data[7];                // II -> Square (X)
            data->gamepad.y = (~ps2Data[4]) & (1 << 4);  // B -> Triangle (Y)
            data->gamepad.leftShoulder = ps2Data[8];
            data->gamepad.rightShoulder = ((~ps2Data[4]) & (1 << 3)) ? 0xFF : 0;
            data->gamepad_pressures.pressureA = ps2Data[6];
            data->gamepad_pressures.pressureB = data->gamepad.b ? 0xFF : 0;
            data->gamepad_pressures.pressureX = ps2Data[7];
            data->gamepad_pressures.pressureY = data->gamepad.y ? 0xFF : 0;
            data->wheel.accelerate = ps2Data[6];
            data->wheel.brake = ps2Data[7];
            break;
        case PSX_GUNCON:
            data->mouse.left = (~ps2Data[3]) & (1 << 3);
            data->mouse.right = (~ps2Data[3]) & (1 << 2);
            data->mouse.x = (ps2Data[6] << 8) | ps2Data[5];
            data->mouse.y = (ps2Data[8] << 8) | ps2Data[7];
            break;
        case PSX_MOUSE:
            data->mouse.left = (~ps2Data[3]) & (1 << 3);
            data->mouse.right = (~ps2Data[3]) & (1 << 2);
            data->mouse.x = (ps2Data[5] - 128) << 8;
            data->mouse.y = -(ps2Data[6] - 127) << 8;
            break;
    }
    if (mType == PSX_JOGCON) {
        data->wheel.wheel = (ps2Data[6] << 8) | ps2Data[5];
    }
    // and then ds1 / ds2 have sticks
    if (mType == PSX_DUALSHOCK_1_CONTROLLER || mType == PSX_DUALSHOCK_2_CONTROLLER) {
        data->gamepad.leftStickX = (ps2Data[7] - 128) << 8;
        data->gamepad.leftStickY = -(ps2Data[8] - 127) << 8;
        data->gamepad.rightStickX = (ps2Data[5] - 128) << 8;
        data->gamepad.rightStickY = -(ps2Data[6] - 127) << 8;
    }
    // and ds2 has analog pressures
    if (mType == PSX_DUALSHOCK_2_CONTROLLER) {
        data->gamepad.leftTrigger = ps2Data[19] << 8;
        data->gamepad.rightTrigger = ps2Data[20] << 8;
        data->gamepad_pressures.pressureA = ps2Data[15] << 8;
        data->gamepad_pressures.pressureB = ps2Data[14] << 8;
        data->gamepad_pressures.pressureX = ps2Data[16] << 8;
        data->gamepad_pressures.pressureY = ps2Data[13] << 8;
        data->gamepad_pressures.pressureDpadUp = ps2Data[11] << 8;
        data->gamepad_pressures.pressureDpadDown = ps2Data[12] << 8;
        data->gamepad_pressures.pressureDpadLeft = ps2Data[10] << 8;
        data->gamepad_pressures.pressureDpadRight = ps2Data[9] << 8;
        data->gamepad_pressures.pressureLeftShoulder = ps2Data[17] << 8;
        data->gamepad_pressures.pressureRightShoulder = ps2Data[18] << 8;
    }
}