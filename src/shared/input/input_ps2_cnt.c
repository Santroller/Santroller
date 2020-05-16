#include "input_ps2_cnt.h"
#include "../config/eeprom.h"
#include "../util.h"
#include "Arduino.h"
#include "arduino_pins.h"
#include <avr/io.h>
#include <math.h>
#include <stdio.h>
#include <util/delay.h>
#include "../output/usb/Descriptors.h"
// PIN: Uno      - SPI PIN - Micro
// CMD: Pin 11   - MOSI    - 16
// DATA: Pin 12  - MISO    - 14
// CLK: Pin 13   - SCK     - 15
// ATT: Pin 10   - ATT     - 10

// Commands for communicating with a PSX controller
static const uint8_t cmd_enter_config[] = {0x01, 0x43, 0x00, 0x01};
static const uint8_t cmd_exit_config[] = {0x01, 0x43, 0x00, 0x00};
static const uint8_t cmd_type_read[] = {0x01, 0x45, 0x00};
static const uint8_t cmd_set_mode[] = {0x01, 0x44, 0x00, /* enabled */ 0x01,
                                       /* locked */ 0x00};

static const uint8_t cmd_set_pressures[] = {0x01, 0x4F, 0x00, 0xFF, 0xFF,
                                            0x03, 0x00, 0x00, 0x00};

static const uint8_t poll[] = {0x01, 0x42, 0x00};

#define INVALID 0xFF
static const uint8_t dsButtons[] = {[PSB_SELECT] = XBOX_BACK,
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

static const uint8_t ghButtons[] = {[PSB_SELECT] = XBOX_BACK,
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

uint8_t ps2_type = PS2_NO_DEVICE;

static inline bool is_valid_reply(const uint8_t *status) {
  return status[1] != 0xFF && (status[2] == 0x5A || status[2] == 0x00);
}

static inline bool is_flight_stick_reply(const uint8_t *status) {
  return (status[1] & 0xF0) == 0x50;
}

static inline bool is_dual_shock_reply(const uint8_t *status) {
  return (status[1] & 0xF0) == 0x70;
}

static inline bool is_dual_shock_2_reply(const uint8_t *status) {
  return status[1] == 0x79;
}

static inline bool is_digital_reply(const uint8_t *status) {
  return (status[1] & 0xF0) == 0x40;
}

static inline bool is_config_reply(const uint8_t *status) {
  return (status[1] & 0xF0) == 0xF0;
}

/** \brief Size of internal communication buffer
 *
 * This can be sized after the longest command reply (which is 21 bytes for
 * 01 42 when in DualShock 2 mode), but we're better safe than sorry.
 */
#define BUFFER_SIZE 32

static uint8_t att_bit;
static volatile uint8_t *att_out;
static uint8_t cmd_bit;
static volatile uint8_t *cmd_out;
static uint8_t clk_bit;
static volatile uint8_t *clk_out;
static uint8_t spsr;
static uint8_t spcr;
static uint8_t spsr_ds1;
static uint8_t spcr_ds1;
static uint8_t spsr_ds2;
static uint8_t spcr_ds2;
void calc_clock (uint32_t clock)  {
  uint8_t clockDiv;
  if (clock >= F_CPU / 2) {
    clockDiv = 0;
  } else if (clock >= F_CPU / 4) {
    clockDiv = 1;
  } else if (clock >= F_CPU / 8) {
    clockDiv = 2;
  } else if (clock >= F_CPU / 16) {
    clockDiv = 3;
  } else if (clock >= F_CPU / 32) {
    clockDiv = 4;
  } else if (clock >= F_CPU / 64) {
    clockDiv = 5;
  } else {
    clockDiv = 7;
  }

  // Invert the SPI2X bit
  clockDiv ^= 0x1;

  // Pack into the SPISettings class
  spcr = _BV(SPE) | _BV(MSTR) | _BV(DORD) | (0x0C) | ((clockDiv >> 1) & 0x03);
  spsr = clockDiv & 0x01;
}

void no_attention(void) {
  uint8_t oldSREG = SREG;
  cli();
  *cmd_out |= cmd_bit;
  *clk_out |= clk_bit;
  *att_out |= att_bit;
  SREG = oldSREG;
}

void shift_in_out(const uint8_t *out, uint8_t *in, const uint8_t len) {
  for (uint8_t i = 0; i < len; ++i) {
    SPDR = out != NULL ? out[i] : 0x5A;
    asm volatile("nop");
    while (!(SPSR & _BV(SPIF)))
      ;
    if (in != NULL) { in[i] = SPDR; }

    _delay_us(INTER_CMD_BYTE_DELAY); // Very important!
  }
}
uint8_t *auto_shift(const uint8_t *out, const uint8_t len) {
  static uint8_t inputBuffer[BUFFER_SIZE];
  uint8_t oldSREG = SREG;
  cli();
  *att_out &= ~att_bit;
  SPCR = spcr;
  SPSR = spsr;
  SREG = oldSREG;
  uint8_t *ret = NULL;

  if (len >= 3 && len <= BUFFER_SIZE) {
    // All commands have at least 3 bytes, so shift out those first
    shift_in_out(out, inputBuffer, 3);
    if (is_valid_reply(inputBuffer)) {
      // Reply is good, get full length

      uint8_t replyLen = (inputBuffer[1] & 0x0F) * 2;

      // Shift out rest of command
      if (len > 3) { shift_in_out(out + 3, inputBuffer + 3, len - 3); }

      uint8_t left = replyLen - len + 3;
      if (left == 0) {
        // The whole reply was gathered
        ret = inputBuffer;
      } else if (len + left <= BUFFER_SIZE) {
        // Part of reply is still missing and we have space for it
        shift_in_out(NULL, inputBuffer + len, left);
        ret = inputBuffer;
      } else {
        // Reply incomplete but not enough space provided
      }
    }
  }
  no_attention();
  return ret;
}
bool send_cmd(const uint8_t *buf, uint8_t len) {
  bool ret = false;

  unsigned long start = millis();
  uint8_t cnt = 0;
  do {
    uint8_t *in = auto_shift(buf, len);

    /* We can't know if we have successfully enabled analog mode until
     * we get out of config mode, so let's just be happy if we get a few
     * consecutive valid replies
     */
    if (in != NULL) {
      ++cnt;
      if (buf == cmd_enter_config) {
        ret = is_config_reply(in);
      } else if (buf == cmd_exit_config) {
        ret = !is_config_reply(in);
      } else {
        ret = cnt >= 3;
      }
    }

    if (!ret) { delay(COMMAND_RETRY_INTERVAL); }
  } while (!ret && millis() - start <= COMMAND_TIMEOUT);

  return ret;
}

uint8_t get_type(void) {
  uint8_t ret = PS2_UNKNOWN_CONTROLLER;

  uint8_t *in = auto_shift(cmd_type_read, sizeof(cmd_type_read));

  if (in != NULL) {
    const uint8_t controllerType = in[3];
    if (controllerType == 0x03) {
      ret = PS2_DUALSHOCK_2_CONTROLLER;
      //~ } else if (controllerType == 0x01 && in[1] == 0x42) {
      //~ return 4;		// ???
    } else if (controllerType == 0x01 && in[1] != 0x42) {
      ret = PS2_GUITAR_HERO_CONTROLLER;
    } else if (controllerType == 0x0C) {
      ret = PS2_WIRELESS_SONY_DUALSHOCK_CONTROLLER;
    }
  }

  return ret;
}

bool read(controller_t *controller) {
  bool ret = false;
  uint8_t *in = auto_shift(poll, sizeof(poll));

  if (in != NULL) {
    if (is_config_reply(in)) {
      // We're stuck in config mode, try to get out
      send_cmd(cmd_exit_config, sizeof(cmd_exit_config));
    } else {
      // We surely have buttons
      uint16_t buttonWord = ~(((uint16_t)in[4] << 8) | in[3]);
      const uint8_t *buttons = dsButtons;
      if (ps2_type == PS2_GUITAR_HERO_CONTROLLER) { buttons = ghButtons; }
      uint8_t btn;
      for (int i = 0; i < XBOX_BTN_COUNT; i++) {
        btn = buttons[i];
        if (btn != INVALID) {
          bit_write(bit_check(buttonWord, i), controller->buttons, btn);
        }
      }

      if (is_dual_shock_reply(in) || is_flight_stick_reply(in)) {
        controller->r_x = (in[5] - 128) << 8;
        controller->r_y = -(in[6] - 127) << 8;
        controller->l_x = (in[7] - 128) << 8;
        controller->l_y = -(in[8] - 127) << 8;
        if (is_dual_shock_2_reply(in)) {
          controller->lt = in[PSAB_L2 + 9];
          controller->rt = in[PSAB_R2 + 9];
          if (ps2_type == PS2_GUITAR_HERO_CONTROLLER) {
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

bool begin(controller_t *controller) {
  spcr = spcr_ds1;
  spsr = spsr_ds1;
  // Some disposable readings to let the controller know we are here
  for (uint8_t i = 0; i < 5; ++i) {
    read(controller);
    delay(1);
  }
  return read(controller);
}

void ps2_cnt_init(void) {
  att_bit = digitalPinToBitMask(10);
  att_out = portOutputRegister(digitalPinToPort(10));
  cmd_bit = digitalPinToBitMask(PIN_SPI_MOSI);
  cmd_out = portOutputRegister(digitalPinToPort(PIN_SPI_MOSI));
  clk_bit = digitalPinToBitMask(PIN_SPI_SCK);
  clk_out = portOutputRegister(digitalPinToPort(PIN_SPI_SCK));
  pinMode(10, OUTPUT);
  pinMode(PIN_SPI_MOSI, OUTPUT);
  pinMode(PIN_SPI_MISO, INPUT_PULLUP);
  pinMode(PIN_SPI_SCK, OUTPUT);
  pinMode(PIN_SPI_SS, OUTPUT);
  no_attention();
  // PS1 controllers don't work with 200000, but PS2 controllers do
  calc_clock(100000);
  spcr_ds1 = spcr;
  spsr_ds1 = spsr;
  calc_clock(200000);
  spcr_ds2 = spcr;
  spsr_ds2 = spsr;
}
void ps2_cnt_tick(controller_t *controller) {
  if (ps2_type == PS2_NO_DEVICE) {
    if (!begin(controller)) { return; }
    // Dualshock one controllers don't have config mode
    if (send_cmd(cmd_enter_config, sizeof(cmd_enter_config))) {
      spcr = spcr_ds2;
      spsr = spsr_ds2;
      ps2_type = get_type();
      // Enable analog sticks
      send_cmd(cmd_set_mode, sizeof(cmd_set_mode));
      // Enable analog buttons
      send_cmd(cmd_set_pressures, sizeof(cmd_set_pressures));
      send_cmd(cmd_exit_config, sizeof(cmd_exit_config));
    } else {
      ps2_type = PS2_DUALSHOCK_1_CONTROLLER;
    }
  }
  if (!read(controller)) { ps2_type = PS2_NO_DEVICE; }
}