#include "ps2_cnt.h"
#include "arduino_pins.h"
#include "config/eeprom.h"
#include "output/descriptors.h"
#include "util/util.h"
#include <avr/io.h>
#include <math.h>
#include <stdio.h>
#include <util/delay.h>
// Commands for communicating with a PSX controller
static const uint8_t commandEnterConfig[] = {0x01, 0x43, 0x00, 0x01};
static const uint8_t commandExitConfig[] = {0x01, 0x43, 0x00, 0x00};
static const uint8_t commandReadType[] = {0x01, 0x45, 0x00};
static const uint8_t commandSetMode[] = {0x01, 0x44, 0x00, /* enabled */ 0x01,
                                         /* locked */ 0x00};

static const uint8_t commandSetPressures[] = {0x01, 0x4F, 0x00, 0xFF, 0xFF,
                                              0x03, 0x00, 0x00, 0x00};

static const uint8_t commandPollInput[] = {0x01, 0x42, 0x00};
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
    [PSB_SELECT] = XBOX_BACK,
    [PSB_L3] = XBOX_LEFT_STICK,
    [PSB_R3] = XBOX_RIGHT_STICK,
    [PSB_START] = XBOX_START,
    [PSB_PAD_UP] = XBOX_DPAD_UP,
    [PSB_PAD_RIGHT] = XBOX_DPAD_RIGHT,
    [PSB_PAD_DOWN] = XBOX_DPAD_DOWN,
    [PSB_PAD_LEFT] = XBOX_DPAD_LEFT,
    [PSB_L2] = INVALID,
    [PSB_R2] = INVALID,
    [PSB_L1] = XBOX_LB,
    [PSB_R1] = XBOX_RB,
    [PSB_TRIANGLE] = XBOX_Y,
    [PSB_CIRCLE] = XBOX_B,
    [PSB_CROSS] = XBOX_A,
    [PSB_SQUARE] = XBOX_X};

static const uint8_t mouseButtonBindings[] = {[PMB_LEFT] = XBOX_A,
                                              [PMB_RIGHT] = XBOX_B};

static const uint8_t guitarHeroButtonBindings[] = {
    [PSB_SELECT] = XBOX_BACK,
    [PSB_L3] = INVALID,
    [PSB_R3] = INVALID,
    [PSB_START] = XBOX_START,
    [PSB_PAD_UP] = XBOX_DPAD_UP,
    [PSB_PAD_RIGHT] = XBOX_DPAD_RIGHT,
    [PSB_PAD_DOWN] = XBOX_DPAD_DOWN,
    [PSB_PAD_LEFT] = XBOX_DPAD_LEFT,
    [GH_STAR_POWER] = INVALID,
    [GH_GREEN] = XBOX_A,
    [PSB_L1] = INVALID,
    [PSB_R1] = INVALID,
    [GH_YELLOW] = XBOX_Y,
    [GH_RED] = XBOX_B,
    [GH_BLUE] = XBOX_X,
    [GH_ORANGE] = XBOX_LB};

uint8_t ps2CtrlType = PSX_NO_DEVICE;

static inline bool isValidReply(const uint8_t *status) {
  return status[1] != 0xFF && (status[2] == 0x5A || status[2] == 0x00);
}

static inline bool isFlightStickReply(const uint8_t *status) {
  return (status[1] & 0xF0) == 0x50;
}

static inline bool isNegconReply(const uint8_t *status) {
  return status[1] == 0x23;
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

static uint8_t attentionBit;
static volatile uint8_t *attentionRegister;
static uint8_t commandBit;
static volatile uint8_t *commandRegister;
static uint8_t clockBit;
static volatile uint8_t *clockRegister;

void noAttention(void) {
  *commandRegister |= commandBit;
  *clockRegister |= clockBit;
  *attentionRegister |= attentionBit;
}

void shiftDataInOut(const uint8_t *out, uint8_t *in, const uint8_t len) {
  for (uint8_t i = 0; i < len; ++i) {
    SPDR = out != NULL ? out[i] : 0x5A;
    asm volatile("nop");
    while (!(SPSR & _BV(SPIF)))
      ;
    if (in != NULL) { in[i] = SPDR; }

    _delay_us(INTER_CMD_BYTE_DELAY); // Very important!
  }
}
uint8_t *autoShiftData(const uint8_t *out, const uint8_t len) {
  static uint8_t inputBuffer[BUFFER_SIZE];
  *attentionRegister &= ~attentionBit;
  _delay_us(ATTN_DELAY);
  uint8_t *ret = NULL;

  if (len >= 3 && len <= BUFFER_SIZE) {
    // All commands have at least 3 bytes, so shift out those first
    shiftDataInOut(out, inputBuffer, 3);
    if (isValidReply(inputBuffer)) {
      // Reply is good, get full length

      uint8_t replyLen = (inputBuffer[1] & 0x0F) * 2;

      // Shift out rest of command
      if (len > 3) { shiftDataInOut(out + 3, inputBuffer + 3, len - 3); }

      uint8_t left = replyLen - len + 3;
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
  }
  noAttention();
  return ret;
}
bool sendCommand(const uint8_t *buf, uint8_t len) {
  bool ret = false;

  unsigned long start = millis();
  uint8_t cnt = 0;
  do {
    uint8_t *in = autoShiftData(buf, len);

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

    if (!ret) { delay(COMMAND_RETRY_INTERVAL); }
  } while (!ret && millis() - start <= COMMAND_TIMEOUT);

  return ret;
}

uint8_t getControllerType(void) {
  uint8_t ret = PSX_UNKNOWN_CONTROLLER;

  uint8_t *in = autoShiftData(commandReadType, sizeof(commandReadType));

  if (in != NULL) {
    const uint8_t controllerType = in[3];
    if (controllerType == 0x03) {
      ret = PSX_DUALSHOCK_2_CONTROLLER;
      //~ } else if (controllerType == 0x01 && in[1] == 0x42) {
      //~ return 4;		// ???
    } else if (controllerType == 0x01 && in[1] != 0x42) {
      ret = PSX_GUITAR_HERO_CONTROLLER;
    } else if (controllerType == 0x0C) {
      ret = PSX_WIRELESS_SONY_DUALSHOCK_CONTROLLER;
    }
  }

  return ret;
}

bool read(Controller_t *controller) {
  bool ret = false;
  uint8_t *in = autoShiftData(commandPollInput, sizeof(commandPollInput));

  if (in != NULL) {
    if (isConfigReply(in)) {
      // We're stuck in config mode, try to get out
      sendCommand(commandExitConfig, sizeof(commandExitConfig));
    } else {
      // We surely have buttons
      uint16_t buttonWord = ~(((uint16_t)in[4] << 8) | in[3]);
      const uint8_t *buttons = dualShockButtonBindings;
      if (ps2CtrlType == PSX_GUITAR_HERO_CONTROLLER) {
        buttons = guitarHeroButtonBindings;
      }

      if (isFlightStickReply(in)) {
        ps2CtrlType = PSX_ANALOG;
      }
      if (isNegconReply(in)) {
        ps2CtrlType = PSX_NEGCON;
        controller->l_x = (in[5] - 128) << 8;
        // These buttons are only analog, map them to digital
        bit_write(in[6] > NEGCON_I_II_BUTTON_THRESHOLD,controller->buttons,XBOX_X);
        bit_write(in[7] > NEGCON_I_II_BUTTON_THRESHOLD,controller->buttons,XBOX_Y);
        bit_write(in[8] > NEGCON_L_BUTTON_THRESHOLD,controller->buttons,XBOX_LB);
      }
      if (isMouseReply(in)) {
        ps2CtrlType = PSX_MOUSE;
        buttons = mouseButtonBindings;
        controller->l_x = (in[5] - 128) << 8;
        controller->l_y = -(in[6] - 127) << 8;
      }
      uint8_t btn;
      for (int i = 0; i < XBOX_BTN_COUNT; i++) {
        btn = buttons[i];
        if (btn != INVALID) {
          bit_write(bit_check(buttonWord, i), controller->buttons, btn);
        }
      }
      if (isDualShockReply(in) || isFlightStickReply(in)) {
        controller->r_x = (in[5] - 128) << 8;
        controller->r_y = -(in[6] - 127) << 8;
        controller->l_x = (in[7] - 128) << 8;
        controller->l_y = -(in[8] - 127) << 8;
        if (isDualShock2Reply(in)) {
          controller->lt = in[PSAB_L2 + 9];
          controller->rt = in[PSAB_R2 + 9];
          if (ps2CtrlType == PSX_GUITAR_HERO_CONTROLLER) {
            controller->r_x = in[GH_WHAMMY + 9];
            controller->r_y = bit_check(buttonWord, GH_STAR_POWER) * 32767;
          }
        }
      }

      ret = true;
    }
  }

  return ret;
}

bool begin(Controller_t *controller) {
  // Some disposable readings to let the controller know we are here
  for (uint8_t i = 0; i < 5; ++i) {
    read(controller);
    delay(1);
  }
  return read(controller);
}

void initPS2CtrlInput(void) {
  attentionBit = digitalPinToBitMask(10);
  attentionRegister = portOutputRegister(digitalPinToPort(10));
  commandBit = digitalPinToBitMask(PIN_SPI_MOSI);
  commandRegister = portOutputRegister(digitalPinToPort(PIN_SPI_MOSI));
  clockBit = digitalPinToBitMask(PIN_SPI_SCK);
  clockRegister = portOutputRegister(digitalPinToPort(PIN_SPI_SCK));
  pinMode(10, OUTPUT);
  noAttention();
}
void tickPS2CtrlInput(Controller_t *controller) {
  if (ps2CtrlType == PSX_NO_DEVICE) {
    if (!begin(controller)) { return; }
    if (sendCommand(commandEnterConfig, sizeof(commandEnterConfig))) {
    // Dualshock one controllers don't have config mode
      ps2CtrlType = getControllerType();
      // Enable analog sticks
      sendCommand(commandSetMode, sizeof(commandSetMode));
      // Enable analog buttons (required for guitar)
      sendCommand(commandSetPressures, sizeof(commandSetPressures));
      sendCommand(commandExitConfig, sizeof(commandExitConfig));
    } else {
      ps2CtrlType = PSX_DUALSHOCK_1_CONTROLLER;
    }
  }
  if (!read(controller)) { ps2CtrlType = PSX_NO_DEVICE; }
}