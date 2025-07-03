#include "ps2.h"

#include <stddef.h>

#include "Arduino.h"
#include "config.h"
#include "io.h"
#include "util.h"
#ifdef INPUT_PS2

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

static bool initialised = false;
static long last = 0;
static uint8_t invalidCount = 0;
void noAttention(void) {
    spi_high(PS2_SPI_PORT);
    INPUT_PS2_ATT_SET();
    delayMicroseconds(ATTN_DELAY);
}
void signalAttention(void) {
    INPUT_PS2_ATT_CLEAR();
    delayMicroseconds(ATTN_DELAY);
}
void shiftDataInOut(const uint8_t *out, uint8_t *in, const uint8_t len) {
    unsigned long m;
    for (uint8_t i = 0; i < len; ++i) {
        uint8_t resp = spi_transfer(PS2_SPI_PORT, out != NULL ? out[i] : 0x5A);
        if (in != NULL) {
            in[i] = resp;
        }

        spi_acknowledged = false;
        m = micros();
        while (!spi_acknowledged) {
            // If for some reason the controller doesn't respond to us, we need to
            // make sure we finish the transfer anyways
            if (micros() - m > INTER_CMD_BYTE_DELAY) {
                break;
            }
        }
    }
}
static uint8_t inputBuffer[BUFFER_SIZE];
static uint8_t inputBuffer2[BUFFER_SIZE];
uint8_t *autoShiftData(uint8_t port, const uint8_t *out, const uint8_t len) {
    uint8_t *ret = NULL;

    if (len >= 2 && len <= BUFFER_SIZE) {
        signalAttention();
        // All commands have at least 3 bytes, so shift out those first
        shiftDataInOut(&port, inputBuffer, 1);
        shiftDataInOut(out, inputBuffer + 1, 2);
        if (isValidReply(inputBuffer)) {
            // Reply is good, get full length
            uint8_t replyLen = (inputBuffer[1] & 0x0F) * 2;

            uint8_t left = replyLen - (len - 2);
            // Shift out rest of command
            shiftDataInOut(out + 2, inputBuffer + 3, len - 2);

            if (left == 0) {
                // The whole reply was gathered
                ret = inputBuffer;
            } else if (len + left <= BUFFER_SIZE) {
                // Part of reply is still missing and we have space for it
                shiftDataInOut(NULL, inputBuffer + len + 1, left);
                ret = inputBuffer;
            } else {
                // Reply incomplete but not enough space provided
            }
        }
        noAttention();
    }
    return ret;
}
bool sendCommand(uint8_t port, const uint8_t *buf, uint8_t len) {
    bool ret = false;
    unsigned long start = millis();
    do {
        uint8_t *in = autoShiftData(port, buf, len);
        /* We can't know if we have successfully enabled analog mode until
         * we get out of config mode, so let's just be happy if we get a few
         * consecutive valid replies
         */
        if (in != NULL) {
            if (buf == commandEnterConfig) {
                ret = isConfigReply(in);
            } else if (buf == commandExitConfig) {
                ret = !isConfigReply(in);
            }
        }

        if (!ret) {
            delay(COMMAND_RETRY_INTERVAL);
        }
    } while (!ret && millis() - start <= COMMAND_TIMEOUT);
    return ret;
}
extern unsigned long millis_at_boot;
uint8_t *tickPS2() {
    uint8_t *in;
    // PS2 guitars die if you poll them too fast
    if ((ps2ControllerType == PSX_GUITAR_HERO_CONTROLLER || ps2ControllerType == PSX_DUALSHOCK_1_CONTROLLER) && micros() - last < 3000) {
        return inputBuffer;
    }
    // If this is changed to a different port, you can talk to different devices
    // on a multitap. Not sure how useful this is unless we make a ps2 variant
    // that works with the multitap, and offers four devices
    uint8_t port = A;
    if (!initialised) {
        if (!autoShiftData(port, commandPollInput, sizeof(commandPollInput))) {
            initialised = false;
            return NULL;
        }
        if (sendCommand(port, commandEnterConfig, sizeof(commandEnterConfig))) {
            // Enable analog sticks
            sendCommand(port, commandSetMode, sizeof(commandSetMode));            
            // Enable pressure sensitive buttons
            // Some PS2 controllers are stupid and *really* need to be told to enable pressure sensitivity
            // for (int i = 0; i < 20; i++) {
            sendCommand(port, commandSetPressures, sizeof(commandSetPressures));
            // }
            sendCommand(port, commandExitConfig, sizeof(commandExitConfig));
        }
        uint8_t *in =
            autoShiftData(port, commandPollInput, sizeof(commandPollInput));
        if (isDualShock2Reply(in)) {
            uint16_t buttonWord = ~(((uint16_t)in[4] << 8) | in[3]);
            if (bit_check(buttonWord, PSB_PAD_LEFT)) {
                ps2ControllerType = PSX_GUITAR_HERO_CONTROLLER;
            } else {
                ps2ControllerType = PSX_DUALSHOCK_2_CONTROLLER;
            }
        } else if (isDualShockReply(in)) {
            uint16_t buttonWord = ~(((uint16_t)in[4] << 8) | in[3]);
            if (bit_check(buttonWord, PSB_PAD_LEFT)) {
                ps2ControllerType = PSX_GUITAR_HERO_CONTROLLER;
            } else {
                ps2ControllerType = PSX_DUALSHOCK_1_CONTROLLER;
            }
        } else if (isFlightStickReply(in)) {
            ps2ControllerType = PSX_FLIGHTSTICK;
        } else if (isNegconReply(in)) {
            ps2ControllerType = PSX_NEGCON;
        } else if (isJogconReply(in)) {
            ps2ControllerType = PSX_JOGCON;
        } else if (isGunconReply(in)) {
            ps2ControllerType = PSX_GUNCON;
        } else if (isMouseReply(in)) {
            ps2ControllerType = PSX_MOUSE;
        } else if (isDigitalReply(in)) {
            ps2ControllerType = PSX_DIGITAL;
        }
        initialised = true;
        invalidCount = 0;
    }
    // Ocassionally, the controller returns a bad packet because it isn't ready. We should ignore that instead of reinitialisng, and
    // We only want to reinit if we recevied several bad packets in a row.
    if (initialised) {
        in = autoShiftData(port, commandPollInput, sizeof(commandPollInput));
        if (in != NULL) {
            invalidCount = 0;
            if (isConfigReply(in)) {
                // We're stuck in config mode, try to get out
                sendCommand(port, commandExitConfig, sizeof(commandExitConfig));
            }
            memcpy(inputBuffer2, inputBuffer, sizeof(inputBuffer));
            last = micros();
        } else {
            invalidCount++;
            if (invalidCount > 10) {
                initialised = false;
            }
            in = inputBuffer2;
        }
    }
    return in;
}
#endif