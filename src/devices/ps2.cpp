#include "devices/ps2.hpp"

#include "interfaces/core.hpp"
static inline bool isValidReply(const uint8_t *status) {
    return status[1] != 0xFF && (status[2] == 0x5A || status[2] == 0x00);
}

static inline bool isFlightStickReply(const uint8_t *status) {
    return (status[1] & 0xF0) == 0x50;
}

static inline bool isNegconReply(const uint8_t *status) {
    return status[1] == 0x23;
}
static inline bool isJogconReply(const uint8_t *status) {
    return (status[1] & 0xF0) == 0xE0;
}

static inline bool isGunconReply(const uint8_t *status) {
    return status[1] == 0x63;
}
static inline bool isMouseReply(const uint8_t *status) {
    return status[1] == 0x12;
}

static inline bool isDualShockReply(const uint8_t *status) {
    return (status[1] & 0xF0) == 0x70;
}

static inline bool isDualShock2Reply(const uint8_t *status) {
    return status[1] == 0x79;
}

static inline bool isDigitalReply(const uint8_t *status) {
    return (status[1] & 0xF0) == 0x40;
}

static inline bool isConfigReply(const uint8_t *status) {
    return (status[1] & 0xF0) == 0xF0;
}
static const uint8_t commandEnterConfig[] = {0x43, 0x00, 0x01, 0x5A,
                                             0x5A, 0x5A, 0x5A, 0x5A};
static const uint8_t commandExitConfig[] = {0x43, 0x00, 0x00, 0x5A,
                                            0x5A, 0x5A, 0x5A, 0x5A};
static const uint8_t commandEnableRumble[] = {0x4d, 0x00, 0x01, 0xff,
                                              0xff, 0xff, 0xff, 0xff};
static const uint8_t commandGetStatus[] = {0x45, 0x00, 0x00, 0x5A,
                                           0x5A, 0x5A, 0x5A, 0x5A};

static const uint8_t commandGetExtra[] = {0x46, 0x00, 0x00, 0x5A,
                                          0x5A, 0xA, 0x5A, 0x5A};
static const uint8_t commandGetExtra2[] = {0x46, 0x00, 0x00, 0x5A,
                                           0x5A, 0xA, 0x5A, 0x5A};

static const uint8_t commandSetMode[] = {0x44, 0x00, /* enabled */ 0x01,
                                         /* locked */ 0x00, 0x02};

// Enable all analog values
static const uint8_t commandSetPressures[] = {0x4F, 0x00, 0xFF, 0xFF,
                                              0x03, 0x00, 0x00, 0x00};

// The pressures data format is 0x4F, 0x00, followed by 18 bits, for each
// of the 18 possible bytes that a controller can return

// For some controllers, we want the buttons (2 bytes, and then 4 bytes of
// sticks (rx, ry, lx, ly)).
static const uint8_t commandSetPressuresSticksOnly[] = {
    0x4F, 0x00, 0b111111, 0x00, 0b00, 0x00, 0x00, 0x00};

// For guitars, we want the buttons (2 bytes, and then ly).
static const uint8_t commandSetPressuresGuitar[] = {0x4F, 0x00, 0b110001, 0x00,
                                                    0b00, 0x00, 0x00, 0x00};

// For the mouse, we want the buttons (2 bytes, and then 2 bytes of axis (x,
// y)). We also want triggers, which luckily happen to be the last two
// bits and hence in their own byte
static const uint8_t commandSetPressuresMouse[] = {0x4F, 0x00, 0b1111, 0x00,
                                                   0b11, 0x00, 0x00, 0x00};

static const uint8_t commandPollInput[] = {0x42, 0x00, 0xFF, 0xFF};

static volatile bool mSpiAcknowledged;
void attentionInterrupt() {
    mSpiAcknowledged = true;
}
PS2Device::PS2Device(SPIMasterInterface *interface, GPIOInterface *csPin, GPIOInterface *attPin) : mInterface(interface), mCsPin(csPin), mAttPin(attPin), mType(PSX_UNKNOWN_CONTROLLER), mFound(false) {
    attPin->registerInterrupt(attentionInterrupt);
}
void PS2Device::noAttention(void) {
    mCsPin->high();
    Core::delayMicroseconds(ATTN_DELAY);
}
void PS2Device::signalAttention(void) {
    mCsPin->low();
    Core::delayMicroseconds(ATTN_DELAY);
}
void PS2Device::shiftDataInOut(const uint8_t *out, uint8_t *in, const uint8_t len) {
    unsigned long m;
    for (uint8_t i = 0; i < len; ++i) {
        uint8_t resp = mInterface->transfer(out != nullptr ? out[i] : 0x5A);
        if (in != nullptr) {
            in[i] = resp;
        }

        mSpiAcknowledged = false;
        m = Core::micros();
        while (!mSpiAcknowledged) {
            // If for some reason the controller doesn't respond to us, we need to
            // make sure we finish the transfer anyways
            if (Core::micros() - m > INTER_CMD_BYTE_DELAY) {
                break;
            }
        }
    }
}
bool PS2Device::autoShiftData(uint8_t port, uint8_t *in, const uint8_t *out, const uint8_t len) {
    uint8_t *ret = nullptr;

    if (len >= 2 && len <= BUFFER_SIZE) {
        signalAttention();
        // All commands have at least 3 bytes, so shift out those first
        shiftDataInOut(&port, in, 1);
        shiftDataInOut(out, in + 1, 2);
        if (isValidReply(in)) {
            // Reply is good, get full length
            uint8_t replyLen = (in[1] & 0x0F) * 2;

            uint8_t left = replyLen - (len - 2);
            // Shift out rest of command
            shiftDataInOut(out + 2, in + 3, len - 2);

            if (left == 0) {
                // The whole reply was gathered
                ret = in;
            } else if (len + left <= BUFFER_SIZE) {
                // Part of reply is still missing and we have space for it
                shiftDataInOut(nullptr, in + len + 1, left);
                ret = in;
            } else {
                // Reply incomplete but not enough space provided
            }
        }
        noAttention();
    }
    return ret;
}
bool PS2Device::sendCommand(uint8_t port, uint8_t *in, const uint8_t *buf, uint8_t len) {
    bool ret = false;
    unsigned long start = Core::millis();
    do {
        /* We can't know if we have successfully enabled analog mode until
         * we get out of config mode, so let's just be happy if we get a few
         * consecutive valid replies
         */
        if (autoShiftData(port, in, buf, len)) {
            if (buf == commandEnterConfig) {
                ret = isConfigReply(in);
            } else if (buf == commandExitConfig) {
                ret = !isConfigReply(in);
            }
        }

        if (!ret) {
            Core::delay(COMMAND_RETRY_INTERVAL);
        }
    } while (!ret && Core::millis() - start <= COMMAND_TIMEOUT);
    return ret;
}
extern unsigned long millis_at_boot;
void PS2Device::tick(san_base_t *data) {
    static uint8_t ps2Data[BUFFER_SIZE];
    // PS2 guitars die if you poll them too fast
    if (mType == PSX_GUITAR_HERO_CONTROLLER && Core::micros() - last < 3000) {
        return;
    }
    // If this is changed to a different port, you can talk to different devices
    // on a multitap. Not sure how useful this is unless we make a ps2 variant
    // that works with the multitap, and offers four devices
    uint8_t port = A;
    if (!mFound) {
        if (!autoShiftData(port, ps2Data, commandPollInput, sizeof(commandPollInput))) {
            return;
        }
        if (sendCommand(port, ps2Data, commandEnterConfig, sizeof(commandEnterConfig))) {
            // Enable analog sticks
            sendCommand(port, ps2Data, commandSetMode, sizeof(commandSetMode));
            // Enable pressure sensitive buttons
            // Some PS2 controllers are stupid and *really* need to be told to enable pressure sensitivity
            // for (int i = 0; i < 20; i++) {
            sendCommand(port, ps2Data, commandSetPressures, sizeof(commandSetPressures));
            // }
            sendCommand(port, ps2Data, commandExitConfig, sizeof(commandExitConfig));
        }
        if (!autoShiftData(port, ps2Data, commandPollInput, sizeof(commandPollInput))) {
            return;
        }
        if (isDualShock2Reply(ps2Data)) {
            // Check if dpad left is held
            if ((~ps2Data[3]) & (1 << 7)) {
                mType = PSX_GUITAR_HERO_CONTROLLER;
            } else {
                mType = PSX_DUALSHOCK_2_CONTROLLER;
            }
        } else if (isDualShockReply(ps2Data)) {
            if ((~ps2Data[3]) & (1 << 7)) {
                mType = PSX_GUITAR_HERO_CONTROLLER;
            } else {
                mType = PSX_DUALSHOCK_1_CONTROLLER;
            }
        } else if (isFlightStickReply(ps2Data)) {
            mType = PSX_FLIGHTSTICK;
        } else if (isNegconReply(ps2Data)) {
            mType = PSX_NEGCON;
        } else if (isJogconReply(ps2Data)) {
            mType = PSX_JOGCON;
        } else if (isGunconReply(ps2Data)) {
            mType = PSX_GUNCON;
        } else if (isMouseReply(ps2Data)) {
            mType = PSX_MOUSE;
        } else if (isDigitalReply(ps2Data)) {
            mType = PSX_DIGITAL;
        }
        mFound = true;
        invalidCount = 0;
    }
    if (mFound) {
        if (autoShiftData(port, ps2Data, commandPollInput, sizeof(commandPollInput))) {
            invalidCount = 0;
            if (isConfigReply(ps2Data)) {
                // We're stuck in config mode, try to get out
                sendCommand(port, ps2Data, commandExitConfig, sizeof(commandExitConfig));
                autoShiftData(port, ps2Data, commandPollInput, sizeof(commandPollInput));
            }
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
            last = Core::micros();
        } else {
            // Ocassionally, the controller returns a bad packet because it isn't ready. We should ignore that instead of reinitialisng, and
            // We only want to reinit if we recevied several bad packets in a row.
            invalidCount++;
            if (invalidCount > 10) {
                mFound = false;
            }
        }
    }
}