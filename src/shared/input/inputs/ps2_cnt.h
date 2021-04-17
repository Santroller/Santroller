#pragma once
#include "controller/controller.h"
#include "eeprom/eeprom.h"
#include "output/descriptors.h"
#include "pins/pins.h"
#include "pins_arduino.h"
#include "spi/spi.h"
#include "timer/timer.h"
#include "util/util.h"
// #include <avr/io.h>
#include "timer/timer.h"
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#ifndef __AVR__
#  include "hardware/clocks.h"
#  include "pico/stdlib.h"
#  include "spi/pio_spi.h"
#endif
// TODO: this seems like a much nicer implementation to copy
// https://github.com/RandomInsano/pscontroller-rs/blob/master/src/lib.rs
/** \brief Command Inter-Byte Delay (us)
 *
 * Commands are several bytes long. This is the time to wait between two
 * consecutive bytes.
 *
 * This should actually be done by watching the \a Acknowledge line, but we are
 * ignoring it at the moment.
 */
#define INTER_CMD_BYTE_DELAY 15

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
#define ATTN_DELAY 15
enum PsxButton {
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
enum GHAnalogButton { GH_WHAMMY = PSAB_L1 };

void tickPS2CtrlInput(Controller_t *controller);
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
// static const uint8_t dualShockButtonBindings[] = {
//     [PSB_SELECT] = XBOX_BACK,
//     [PSB_L3] = XBOX_LEFT_STICK,
//     [PSB_R3] = XBOX_RIGHT_STICK,
//     [PSB_START] = XBOX_START,
//     [PSB_PAD_UP] = XBOX_DPAD_UP,
//     [PSB_PAD_RIGHT] = XBOX_DPAD_RIGHT,
//     [PSB_PAD_DOWN] = XBOX_DPAD_DOWN,
//     [PSB_PAD_LEFT] = XBOX_DPAD_LEFT,
//     [PSB_L2] = INVALID,
//     [PSB_R2] = INVALID,
//     [PSB_L1] = XBOX_LB,
//     [PSB_R1] = XBOX_RB,
//     [PSB_TRIANGLE] = XBOX_Y,
//     [PSB_CIRCLE] = XBOX_B,
//     [PSB_CROSS] = XBOX_A,
//     [PSB_SQUARE] = XBOX_X};

// static const uint8_t mouseButtonBindings[] = {
//     [PMB_LEFT] = XBOX_A, [PMB_RIGHT] = XBOX_B};

// static const uint8_t guitarHeroButtonBindings[] = {
//     [PSB_SELECT] = XBOX_BACK,
//     [PSB_L3] = INVALID,
//     [PSB_R3] = INVALID,
//     [PSB_START] = XBOX_START,
//     [PSB_PAD_UP] = XBOX_DPAD_UP,
//     [PSB_PAD_RIGHT] = XBOX_DPAD_RIGHT,
//     [PSB_PAD_DOWN] = XBOX_DPAD_DOWN,
//     [PSB_PAD_LEFT] = XBOX_DPAD_LEFT,
//     [GH_STAR_POWER] = INVALID,
//     [GH_GREEN] = XBOX_A,
//     [PSB_L1] = INVALID,
//     [PSB_R1] = INVALID,
//     [GH_YELLOW] = XBOX_Y,
//     [GH_RED] = XBOX_B,
//     [GH_BLUE] = XBOX_X,
//     [GH_ORANGE] = XBOX_LB};
// Inverse of above
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
    [XBOX_DPAD_UP] = PSB_PAD_UP,
    [XBOX_DPAD_DOWN] = PSB_PAD_DOWN,
    [XBOX_DPAD_LEFT] = PSB_PAD_LEFT,
    [XBOX_DPAD_RIGHT] = PSB_PAD_RIGHT,
    [XBOX_START] = PSB_START,
    [XBOX_BACK] = PSB_SELECT,
    [XBOX_LEFT_STICK] = INVALID,
    [XBOX_RIGHT_STICK] = INVALID,
    [XBOX_LB] = GH_ORANGE,
    [XBOX_RB] = INVALID,
    [XBOX_HOME] = INVALID,
    [XBOX_UNUSED] = INVALID,
    [XBOX_A] = GH_GREEN,
    [XBOX_B] = GH_RED,
    [XBOX_X] = GH_BLUE,
    [XBOX_Y] = GH_YELLOW};
uint16_t lastButtons;
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
Pin_t command;
Pin_t clock;

#ifndef __AVR__
pio_spi_inst_t spi = {.pio = pio0, .sm = 0};
#endif
void noAttention(void) {
  digitalWritePin(command, true);
  digitalWritePin(clock, true);
  digitalWritePin(attention, true);
  _delay_us(ATTN_DELAY);
}
void signalAttention(void) {
  digitalWritePin(attention, false);
  digitalWritePin(clock, false);
  digitalWritePin(command, false);
  _delay_us(ATTN_DELAY);
}
void shiftDataInOut(const uint8_t *out, uint8_t *in, const uint8_t len) {
  for (uint8_t i = 0; i < len; ++i) {
    uint8_t resp = spi_transfer(out != NULL ? out[i] : 0x5A);
    if (in != NULL) { in[i] = resp; }
    _delay_us(INTER_CMD_BYTE_DELAY); // Very important!
  }
}
uint8_t *autoShiftData(const uint8_t *out, const uint8_t len) {

  static uint8_t inputBuffer[BUFFER_SIZE];
  uint8_t *ret = NULL;
#ifdef __AVR__
  signalAttention();

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
#else
  // We can parse commands in PIO, and then automagically read the entire packet
  pio_spi_write8_read8_blocking(&spi, out, inputBuffer, len);
  for (int i = 0; i < len; i++) {
    printf("%x ", inputBuffer[i]);
  }
  printf("\n");
  ret = inputBuffer;
#endif
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

    if (!ret) { _delay_ms(COMMAND_RETRY_INTERVAL); }
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
uint16_t buttonWord;
bool read(Controller_t *controller) {
  bool ret = false;
  uint8_t *in = autoShiftData(commandPollInput, sizeof(commandPollInput));

  if (in != NULL) {
    if (isConfigReply(in)) {
      // We're stuck in config mode, try to get out
      sendCommand(commandExitConfig, sizeof(commandExitConfig));
    } else {
      // We surely have buttons
      buttonWord = ~(((uint16_t)in[4] << 8) | in[3]);

      if (isFlightStickReply(in)) { ps2CtrlType = PSX_ANALOG; }
      if (isNegconReply(in)) {
        ps2CtrlType = PSX_NEGCON;
        controller->l_x = (in[5] - 128) << 8;
        // These buttons are only analog, map them to digital
        bit_write(in[6] > NEGCON_I_II_BUTTON_THRESHOLD, controller->buttons,
                  XBOX_X);
        bit_write(in[7] > NEGCON_I_II_BUTTON_THRESHOLD, controller->buttons,
                  XBOX_Y);
        bit_write(in[8] > NEGCON_L_BUTTON_THRESHOLD, controller->buttons,
                  XBOX_LB);
      }
      if (isJogconReply(in)) {
        ps2CtrlType = PSX_JOGCON;
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
        controller->l_x = in[5];
        if (in[6] < 0x80) {
          // CW up to half
          controller->l_x = in[5] < 0x80 ? in[5] : (0x80 - 1);
        } else {
          // CCW down to half
          controller->l_x = in[5] > 0x80 ? in[5] : (0x80 + 1);
        }

        // Bring to the usual 0-255 range
        controller->l_x += 0x80;
      }
      if (isMouseReply(in)) {
        ps2CtrlType = PSX_MOUSE;
        controller->l_x = (in[5] - 128) << 8;
        controller->l_y = -(in[6] - 127) << 8;
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
    _delay_ms(1);
  }
  return read(controller);
}

void initPS2CtrlInput(Configuration_t *config) {
#ifdef __AVR__
  spi_begin(100000, true, true, true);
  attention = setUpDigital(config, PIN_PS2_ATT, 0, false, true);
  command = setUpDigital(config, PIN_SPI_MOSI, 0, false, true);
  clock = setUpDigital(config, PIN_SPI_SCK, 0, false, true);
  pinMode(PIN_PS2_ATT, OUTPUT);
  noAttention();
#else
  pinMode(PIN_PS2_ATT, OUTPUT);
  gpio_put(PIN_PS2_ATT, 1);
  float clkdiv = clock_get_hz(clk_sys) / 10000.f;
  uint cpha1_prog_offs = pio_add_program(spi.pio, &spi_cpha1_program);
  pio_spi_init(spi.pio, spi.sm, cpha1_prog_offs,
                  8, // 8 bits per SPI frame
                  clkdiv, 1, 1, PIN_SPI_SCK, PIN_SPI_MOSI, PIN_SPI_MISO);
#endif
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

bool readPS2Button(Pin_t pin) {
  const uint8_t *buttons = dualShockButtonBindings;
  if (ps2CtrlType == PSX_GUITAR_HERO_CONTROLLER) {
    buttons = guitarHeroButtonBindings;
  } else if (ps2CtrlType == PSX_MOUSE) {
    buttons = mouseButtonBindings;
  }
  uint8_t btn = buttons[pin.offset];
  if (btn == INVALID) { return false; }
  return !!bit_check(buttonWord, btn);
}