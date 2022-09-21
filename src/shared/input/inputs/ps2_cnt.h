#pragma once
#include "controller/controller.h"
#include "eeprom/eeprom.h"
#include "output/descriptors.h"
#include "pins/pins.h"
#include "pins_arduino.h"
#include "spi/spi.h"
#include "timer/timer.h"
#include "util/util.h"
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

/** \brief Command Inter-Byte Delay (us)
 *
 * Commands are several bytes long. This is the time to wait between two
 * consecutive bytes.
 *
 * This should actually be done by watching the \a Acknowledge line, but we are
 * ignoring it at the moment.
 */
#define INTER_CMD_BYTE_DELAY 50
/** \brief Command timeout (ms)
 *
 * Commands are sent to the controller repeatedly, until they succeed or time
 * out. This is the length of that timeout.
 *
 * \sa COMMAND_RETRY_INTERVAL
 */
#define COMMAND_TIMEOUT 250

/** \brief Command Retry Interval (ms)
 *
 * When sending a command to the controller, if it does not succeed, it is
 * retried after this amount of time.
 */
#define COMMAND_RETRY_INTERVAL 10

/** \brief Attention Delay
 *
 * Time between attention being issued to the controller and the first clock
 * edge (us).
 */
#define ATTN_DELAY 50

enum PsxButton {
  // PSB_NONE,
  PSB_SELECT,
  PSB_L3,
  PSB_R3,
  PSB_START,
  PSB_PAD_UP,
  PSB_PAD_RIGHT,
  PSB_PAD_DOWN,
  PSB_PAD_LEFT,
  PSB_L2,
  PSB_R2,
  PSB_L1,
  PSB_R1,
  PSB_TRIANGLE,
  PSB_CIRCLE,
  PSB_CROSS,
  PSB_SQUARE
};

enum MouseButton {
  PMB_LEFT = 9,
  PMB_RIGHT,
};

enum GHButton {
  GH_STAR_POWER = PSB_L2,
  GH_GREEN = PSB_R2,
  GH_RED = PSB_CIRCLE,
  GH_YELLOW = PSB_TRIANGLE,
  GH_BLUE = PSB_CROSS,
  GH_ORANGE = PSB_SQUARE
};

enum PsxAnalogButton {
  PSAB_PAD_RIGHT,
  PSAB_PAD_LEFT,
  PSAB_PAD_UP,
  PSAB_PAD_DOWN,
  PSAB_TRIANGLE,
  PSAB_CIRCLE,
  PSAB_CROSS,
  PSAB_SQUARE,
  PSAB_L1,
  PSAB_R1,
  PSAB_L2,
  PSAB_R2
};
enum MultitapPort { A = 0x01, B = 0x02, C = 0x03, D = 0x04 };

void tickPS2CtrlInput(Controller_t *controller);
// Commands for communicating with a PSX controller
static const uint8_t commandEnterConfig[] = {0x43, 0x00, 0x01, 0x5A,
                                             0x5A, 0x5A, 0x5A, 0x5A};
static const uint8_t commandExitConfig[] = {0x43, 0x00, 0x00, 0x5A,
                                            0x5A, 0x5A, 0x5A, 0x5A};
static const uint8_t commandGetStatus[] = {0x45, 0x00, 0x00, 0x5A,
                                           0x5A, 0x5A, 0x5A, 0x5A};
static const uint8_t commandSetMode[] = {0x44, 0x00, /* enabled */ 0x01,
                                         /* locked */ 0x00};

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
                                                    0b00, 0x00, 0x00,     0x00};

// For the ds2, we want the buttons (2 bytes, and then 4 bytes of sticks (rx,
// ry, lx, ly)). We also want triggers, which luckily happen to be the last two
// bits and hence in their own byte
static const uint8_t commandSetPressuresDS2[] = {0x4F, 0x00, 0b111111, 0x00,
                                                 0b11, 0x00, 0x00,     0x00};

// For the mouse, we want the buttons (2 bytes, and then 2 bytes of axis (x,
// y)). We also want triggers, which luckily happen to be the last two
// bits and hence in their own byte
static const uint8_t commandSetPressuresMouse[] = {0x4F, 0x00, 0b1111, 0x00,
                                                   0b11, 0x00, 0x00,   0x00};

static const uint8_t commandPollInput[] = {0x42, 0x00, 0xFF, 0xFF};

/** \brief neGcon I/II-button press threshold
 *
 * The neGcon does not report digital button press data for its analog buttons,
 * so we have to make it up. The Square, Cross digital buttons will be
 * reported as pressed when the analog value of the II and I buttons
 * (respectively), goes over this threshold.
 *
 * \sa NEGCON_L_BUTTON_THRESHOLD
 */
const uint8_t NEGCON_I_II_BUTTON_THRESHOLD = 128U;

/** \brief neGcon L-button press threshold
 *
 * The neGcon does not report digital button press data for its analog buttons,
 * so we have to make it up. The L1 digital button will be reported as pressed
 * when the analog value of the L buttons goes over this threshold.
 *
 * This value has been tuned so that the L button gets digitally triggered at
 * about the same point as the non-analog R button. This is done "empirically"
 * and might need tuning on a different controller than the one I actually have.
 *
 * \sa NEGCON_I_II_BUTTON_THRESHOLD
 */
const uint8_t NEGCON_L_BUTTON_THRESHOLD = 240U;
#define INVALID 0xFF
static const uint8_t dualShockButtonBindings[] = {
    [XBOX_DPAD_UP] = PSB_PAD_UP,
    [XBOX_DPAD_DOWN] = PSB_PAD_DOWN,
    [XBOX_DPAD_LEFT] = PSB_PAD_LEFT,
    [XBOX_DPAD_RIGHT] = PSB_PAD_RIGHT,
    [XBOX_START] = PSB_START,
    [XBOX_BACK] = PSB_SELECT,
    [XBOX_LEFT_STICK] = PSB_L3,
    [XBOX_RIGHT_STICK] = PSB_R3,
    [XBOX_LB] = PSB_L1,
    [XBOX_RB] = PSB_R1,
    [XBOX_HOME] = INVALID,
    [XBOX_UNUSED] = INVALID,
    [XBOX_A] = PSB_CROSS,
    [XBOX_B] = PSB_CIRCLE,
    [XBOX_X] = PSB_SQUARE,
    [XBOX_Y] = PSB_TRIANGLE};

static const uint8_t mouseButtonBindings[] = {
    [XBOX_DPAD_UP] = INVALID,    [XBOX_DPAD_DOWN] = INVALID,
    [XBOX_DPAD_LEFT] = INVALID,  [XBOX_DPAD_RIGHT] = INVALID,
    [XBOX_START] = INVALID,      [XBOX_BACK] = INVALID,
    [XBOX_LEFT_STICK] = INVALID, [XBOX_RIGHT_STICK] = INVALID,
    [XBOX_LB] = INVALID,         [XBOX_RB] = INVALID,
    [XBOX_HOME] = INVALID,       [XBOX_UNUSED] = INVALID,
    [XBOX_A] = PMB_LEFT,         [XBOX_B] = PMB_RIGHT,
    [XBOX_X] = INVALID,          [XBOX_Y] = INVALID};

static const uint8_t guitarHeroButtonBindings[] = {
    [XBOX_DPAD_UP] = PSB_PAD_UP, [XBOX_DPAD_DOWN] = PSB_PAD_DOWN,
    [XBOX_DPAD_LEFT] = INVALID,  [XBOX_DPAD_RIGHT] = INVALID,
    [XBOX_START] = PSB_START,    [XBOX_BACK] = PSB_SELECT,
    [XBOX_LEFT_STICK] = INVALID, [XBOX_RIGHT_STICK] = INVALID,
    [XBOX_LB] = GH_ORANGE,       [XBOX_RB] = INVALID,
    [XBOX_HOME] = INVALID,       [XBOX_UNUSED] = INVALID,
    [XBOX_A] = GH_GREEN,         [XBOX_B] = GH_RED,
    [XBOX_X] = GH_BLUE,          [XBOX_Y] = GH_YELLOW};
uint16_t lastButtons;
uint8_t ps2CtrlType = PSPROTO_NO_DEVICE;

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

/** \brief Size of internal communication buffer
 *
 * This can be sized after the longest command reply (which is 21 bytes for
 * 01 42 when in DualShock 2 mode), but we're better safe than sorry.
 */
#define BUFFER_SIZE 32

Pin_t attention;
Pin_t acknowledge;
Pin_t command;
Pin_t clock;
void noAttention(void) {
  spi_high();
  digitalWritePin(&attention, true);
  _delay_us(ATTN_DELAY);
}
void signalAttention(void) {
  digitalWritePin(&attention, false);
  _delay_us(ATTN_DELAY);
}
void shiftDataInOut(const uint8_t *out, uint8_t *in, const uint8_t len) {
  unsigned long m;
  for (uint8_t i = 0; i < len; ++i) {
    uint8_t resp = spi_transfer(out != NULL ? out[i] : 0x5A);
    // printf("%02x %02x\n", out != NULL ? out[i] : 0x5A, resp);
    if (in != NULL) { in[i] = resp; }

    spi_acknowledged = false;
    m = micros();
    while (!spi_acknowledged) {
      // If for some reason the controller doesn't respond to us, we need to
      // make sure we finish the transfer anyways
      if (micros() - m > INTER_CMD_BYTE_DELAY) { break; }
    }
  }
}
uint8_t *autoShiftData(uint8_t port, const uint8_t *out, const uint8_t len) {

  static uint8_t inputBuffer[BUFFER_SIZE];
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
        shiftDataInOut(NULL, inputBuffer + len, left);
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
  uint8_t cnt = 0;
  do {
    uint8_t *in = autoShiftData(port, buf, len);
    /* We can't know if we have successfully enabled analog mode until
     * we get out of config mode, so let's just be happy if we get a few
     * consecutive valid replies
     */
    if (in != NULL) {
      ++cnt;
      if (buf == commandEnterConfig) {
        ret = isConfigReply(in);
      } else if (buf == commandExitConfig) {
        ret = !isConfigReply(in);
      } else {
        ret = cnt >= 3;
      }
    }

    if (!ret) { _delay_ms(COMMAND_RETRY_INTERVAL); }
  } while (!ret && millis() - start <= COMMAND_TIMEOUT);
  return ret;
}
uint16_t buttonWord;
bool read(uint8_t port, Controller_t *controller) {
  uint8_t *in = autoShiftData(port, commandPollInput, sizeof(commandPollInput));
  if (in != NULL) {
    if (isConfigReply(in)) {
      // We're stuck in config mode, try to get out
      sendCommand(port, commandExitConfig, sizeof(commandExitConfig));
    } else {
      // We surely have buttons
      buttonWord = ~(((uint16_t)in[4] << 8) | in[3]);

      switch (ps2CtrlType) {
      case PSPROTO_DUALSHOCK2: {
        controller->lt = in[9];
        controller->rt = in[10];
        /* Now fall through to DualShock case */
      }
      case PSPROTO_GUNCON:
        /* The Guncon uses the same reply format as DualShocks,
         * by just falling through we'll end up with:
         * - A (Left side) -> Start
         * - B (Right side) -> Cross
         * - Trigger -> Circle
         * - Low byte of HSYNC -> RX
         * - High byte of HSYNC -> RY
         * - Low byte of VSYNC -> LX
         * - High byte of VSYNC -> LY
         */
      case PSPROTO_DUALSHOCK:
      case PSPROTO_FLIGHTSTICK: {
        // We have analog stick data
        controller->r_x = (in[5] - 128) << 8;
        controller->r_y = -(in[6] - 127) << 8;
        controller->l_x = (in[7] - 128) << 8;
        controller->l_y = -(in[8] - 127) << 8;
        break;
      }
      case PSPROTO_MOUSE: {
        controller->l_x = (in[5] - 128) << 8;
        controller->l_y = -(in[6] - 127) << 8;
        break;
      }
      case PSPROTO_NEGCON: {
        // Map the twist axis to X axis of left analog
        controller->l_x = (in[5] - 128) << 8;

        // Make up "missing" digital data
        if (in[6] >= NEGCON_I_II_BUTTON_THRESHOLD) {
          buttonWord &= ~PSB_SQUARE;
        }
        if (in[7] >= NEGCON_I_II_BUTTON_THRESHOLD) { buttonWord &= ~PSB_CROSS; }
        if (in[8] >= NEGCON_L_BUTTON_THRESHOLD) { buttonWord &= ~PSB_L1; }
        break;
      }
      case PSPROTO_JOGCON: {
        /* Map the wheel X axis of left analog, half a rotation
         * per direction: byte 5 has the wheel position, it is
         * 0 at startup, then we have 0xFF down to 0x80 for
         * left/CCW, and 0x01 up to 0x80 for right/CW
         *
         * byte 6 is the number of full CW rotations
         * byte 7 is 0 if wheel is still, 1 if it is rotating CW
         *        and 2 if rotation CCW
         * byte 8 seems to stay at 0
         *
         * We'll want to cap the movement halfway in each
         * direction, for ease of use/implementation.
         */
        int16_t test = (in[5] - 128) << 8;
        if (in[6] < 0x80) {
          // CW up to half
          controller->l_x = in[5] < 0x80 ? test : -1;
        } else {
          // CCW down to half
          controller->l_x = in[5] > 0x80 ? test : 1;
        }

        controller->l_x += 0x80;
        break;
      }
      case PSPROTO_GUITAR: {
        controller->l_x = 0;
        controller->l_y = 0;
        controller->r_x = -(in[8] - 127) << 8;
        controller->r_y = (!!bit_check(buttonWord, GH_STAR_POWER)) * 32767;
        break;
      }
      default:
        break;
      }
    }

    return true;
  }
  return false;
}
bool begin(uint8_t port, Controller_t *controller) {
  // Some disposable readings to let the controller know we are here
  for (uint8_t i = 0; i < 5; ++i) {
    read(port, controller);
    _delay_ms(1);
  }
  return read(port, controller);
}

void initPS2CtrlInput(Configuration_t *config) {
  spi_begin(100000, true, true, true);
  setUpDigital(&attention, config, PIN_PS2_ATT, 0, false, true);
  setUpDigital(&acknowledge, config, PIN_PS2_ACK, 0, true, false);
  pinMode(PIN_PS2_ATT, OUTPUT);
  pinMode(PIN_PS2_ACK, INPUT);
  init_ack(acknowledge);
  noAttention();
}
bool initialised = false;
void tickPS2CtrlInput(Controller_t *controller) {
  // If this is changed to a different port, you can talk to different devices
  // on a multitap. Not sure how useful this is unless we make a ps2 variant
  // that works with the multitap, and offers four devices
  uint8_t port = A;
  if (!initialised) {
    if (!begin(port, controller)) {
      initialised = false;
      return;
    }
    if (sendCommand(port, commandEnterConfig, sizeof(commandEnterConfig))) {
      // Enable analog sticks
      sendCommand(port, commandSetMode, sizeof(commandSetMode));
      // Enable analog buttons

      // Enable pressure sensitive buttons, enable them all as we want to test
      // specifically the analog buttons, as that is how you differenciate
      // dualshock one from dualshock 2
      sendCommand(port, commandSetPressures, sizeof(commandSetPressures));
      sendCommand(port, commandExitConfig, sizeof(commandExitConfig));
    }
    uint8_t *in =
        autoShiftData(port, commandPollInput, sizeof(commandPollInput));

    if (isDualShock2Reply(in)) {
      ps2CtrlType = PSPROTO_DUALSHOCK2;
    } else if (isDualShockReply(in)) {
      ps2CtrlType = PSPROTO_DUALSHOCK;
    } else if (isFlightStickReply(in)) {
      ps2CtrlType = PSPROTO_FLIGHTSTICK;
    } else if (isNegconReply(in)) {
      ps2CtrlType = PSPROTO_NEGCON;
    } else if (isJogconReply(in)) {
      ps2CtrlType = PSPROTO_JOGCON;
    } else if (isGunconReply(in)) {
      ps2CtrlType = PSPROTO_GUNCON;
    } else if (isMouseReply(in)) {
      ps2CtrlType = PSPROTO_MOUSE;
    } else if (isDigitalReply(in)) {
      ps2CtrlType = PSPROTO_DIGITAL;
    }
    if (sendCommand(port, commandEnterConfig, sizeof(commandEnterConfig))) {
      if (ps2CtrlType == PSPROTO_DUALSHOCK) {
        in = autoShiftData(port, commandGetStatus, sizeof(commandGetStatus));
        if (in[3] == 0x01) { ps2CtrlType = PSPROTO_GUITAR; }
      } else if (ps2CtrlType == PSPROTO_FLIGHTSTICK || ps2CtrlType == PSPROTO_GUNCON ||
          ps2CtrlType == PSPROTO_JOGCON || ps2CtrlType == PSPROTO_NEGCON) {
        sendCommand(port, commandSetPressuresSticksOnly,
                    sizeof(commandSetPressuresSticksOnly));
      } else if (ps2CtrlType == PSPROTO_MOUSE) {
        // Mouse is its own thing for speed
        sendCommand(port, commandSetPressuresMouse,
                    sizeof(commandSetPressuresMouse));
      } else if (ps2CtrlType == PSPROTO_DUALSHOCK2) {
        // Dualshock 2 is its own thing, it has analog buttons that we want to
        // disable, but we still want triggers
        sendCommand(port, commandSetPressuresDS2,
                    sizeof(commandSetPressuresDS2));
      }
      sendCommand(port, commandExitConfig, sizeof(commandExitConfig));
    }
    initialised = true;
  }
  if (initialised && !read(port, controller)) { initialised = false; }
}

bool readPS2Button(Pin_t *pin) {
  const uint8_t *buttons = dualShockButtonBindings;
  if (ps2CtrlType == PSPROTO_GUITAR) {
    buttons = guitarHeroButtonBindings;
  } else if (ps2CtrlType == PSPROTO_MOUSE) {
    buttons = mouseButtonBindings;
  }
  uint8_t btn = buttons[pin->offset];
  if (btn == INVALID) { return false; }
  return !!bit_check(buttonWord, btn);
}