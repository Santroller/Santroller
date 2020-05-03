#include "input_ps2_cnt.h"
#include "../util.h"
#include "Arduino.h"
#include "arduino_pins.h"
#include <avr/io.h>
#include <math.h>
#include <stdio.h>
#include <util/delay.h>
//PIN: Uno      - SPI PIN - Micro
//CMD: Pin 11   - MOSI    - 16
//DATA: Pin 12  - MISO    - 14
//CLK: Pin 13   - SCK     - 15
//ATT: Pin 10   - ATT     - 10

// Commands for communicating with a PSX controller
static uint8_t cmd_enter_config[] = {0x01, 0x43, 0x00, 0x01, 0x5A,
                                 0x5A, 0x5A, 0x5A, 0x5A};
static uint8_t cmd_exit_config[] = {0x01, 0x43, 0x00, 0x00, 0x5A,
                                0x5A, 0x5A, 0x5A, 0x5A};
static uint8_t cmd_type_read[] = {0x01, 0x45, 0x00, 0x5A, 0x5A,
                              0x5A, 0x5A, 0x5A, 0x5A};
static uint8_t cmd_set_mode[] = {
    0x01, 0x44, 0x00, /* enabled */ 0x01, /* locked */ 0x00, 0x00,
    0x00, 0x00, 0x00};

static uint8_t cmd_set_pressures[] = {0x01, 0x4F, 0x00, 0xFF, 0xFF,
                                  0x03, 0x00, 0x00, 0x00};

static uint8_t poll[] = {0x01, 0x42, 0x00, 0xFF, 0xFF};

bool isValidReply(const uint8_t *status) {
  return status[1] != 0xFF && (status[2] == 0x5A || status[2] == 0x00);
}

inline bool isFlightstickReply(const uint8_t *status) {
  return (status[1] & 0xF0) == 0x50;
}

inline bool isDualShockReply(const uint8_t *status) {
  return (status[1] & 0xF0) == 0x70;
}

inline bool isDualShock2Reply(const uint8_t *status) {
  return status[1] == 0x79;
}

inline bool isDigitalReply(const uint8_t *status) {
  return (status[1] & 0xF0) == 0x40;
}

inline bool isConfigReply(const uint8_t *status) {
  return (status[1] & 0xF0) == 0xF0;
}

/** \brief Size of internal communication buffer
 *
 * This can be sized after the longest command reply (which is 21 bytes for
 * 01 42 when in DualShock 2 mode), but we're better safe than sorry.
 */
#define BUFFER_SIZE 32

/** \brief Size of buffer holding analog button data
 */
#define ANALOG_BTN_DATA_SIZE 12

/** \brief Internal communication buffer
 *
 * This is used to hold replies received from the controller.
 */
uint8_t inputBuffer[BUFFER_SIZE];

/** \brief Previous (Digital) Button status
 *
 * The individual bits can be identified through #PsxButton.
 */
PsxButtons previousButtonWord;

/** \brief (Digital) Button status
 *
 * The individual bits can be identified through #PsxButton.
 */
PsxButtons buttonWord;
//! @}

/** \brief Analog Button Data
 *
 * \todo What's the meaning of every individual byte?
 */
uint8_t analogButtonData[ANALOG_BTN_DATA_SIZE];

/** \brief Analog Button Data Validity
 *
 * True if the #analogButtonData were valid in last call to read()
 */
bool analogButtonDataValid;
uint8_t att_bit;
volatile uint8_t *att_out;
uint8_t cmd_bit;
volatile uint8_t *cmd_out;
uint8_t clk_bit;
volatile uint8_t *clk_out;
uint8_t ss_bit;
volatile uint8_t *ss_out;
uint8_t spsr;
uint8_t spcr;
uint8_t spsr_ds1;
uint8_t spcr_ds1;
uint8_t spsr_ds2;
uint8_t spcr_ds2;
void ps2_cnt_init(void) {
  att_bit = digitalPinToBitMask(10);
  att_out = portOutputRegister(digitalPinToPort(10));
  cmd_bit = digitalPinToBitMask(PIN_SPI_MOSI);
  cmd_out = portOutputRegister(digitalPinToPort(PIN_SPI_MOSI));
  clk_bit = digitalPinToBitMask(PIN_SPI_SCK);
  clk_out = portOutputRegister(digitalPinToPort(PIN_SPI_SCK));
  ss_bit = digitalPinToBitMask(PIN_SPI_SS);
  ss_out = portOutputRegister(digitalPinToPort(PIN_SPI_SS));
  pinMode(10, OUTPUT);
  pinMode(PIN_SPI_MOSI, OUTPUT);
  pinMode(PIN_SPI_MISO, INPUT_PULLUP);
  pinMode(PIN_SPI_SCK, OUTPUT);
  pinMode(PIN_SPI_SS, OUTPUT);
  uint8_t oldSREG = SREG;
  cli();
  *cmd_out |= cmd_bit;
  *clk_out |= clk_bit;
  *att_out |= att_bit;
  SPCR |= _BV(MSTR);
  SPCR |= _BV(SPE);
  SREG = oldSREG;
  uint8_t clockDiv;
  // TODO: Does this limit poll rate much? Supporting PS1 controllers is nice,
  // but we can double this clock when using PS2 controllers.
  // TODO: If this does make a difference, could we just reconfigure the clock
  // rate depending on what controller we detect?
  uint32_t clock = 100000;
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
    clockDiv = 6;
  }
  // Compensate for the duplicate fosc/64
  if (clockDiv == 6) clockDiv = 7;

  // Invert the SPI2X bit
  clockDiv ^= 0x1;

  // Pack into the SPISettings class
  spcr_ds1 =
      _BV(SPE) | _BV(MSTR) | _BV(DORD) | (0x0C) | ((clockDiv >> 1) & 0x03);
  spsr_ds1 = clockDiv & 0x01;
  clock = 200000;
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
    clockDiv = 6;
  }
  // Compensate for the duplicate fosc/64
  if (clockDiv == 6) clockDiv = 7;

  // Invert the SPI2X bit
  clockDiv ^= 0x1;

  // Pack into the SPISettings class
  spcr_ds2 =
      _BV(SPE) | _BV(MSTR) | _BV(DORD) | (0x0C) | ((clockDiv >> 1) & 0x03);
  spsr_ds2 = clockDiv & 0x01;
}

bool enter_config_mode(void) {
  bool ret = false;

  unsigned long start = millis();
  do {
    attention();
    uint8_t *in = autoShift(cmd_enter_config, 4);
    no_attention();

    ret = in != NULL && isConfigReply(in);

    if (!ret) { delay(COMMAND_RETRY_INTERVAL); }
  } while (!ret && millis() - start <= COMMAND_TIMEOUT);

  return ret;
}

bool enable_analog_sticks(void) {
  bool ret = false;
  uint8_t out[sizeof(cmd_set_mode)];

  memcpy(out, cmd_set_mode, sizeof(cmd_set_mode));

  unsigned long start = millis();
  uint8_t cnt = 0;
  do {
    attention();
    uint8_t *in = autoShift(out, 5);
    no_attention();

    /* We can't know if we have successfully enabled analog mode until
     * we get out of config mode, so let's just be happy if we get a few
     * consecutive valid replies
     */
    if (in != NULL) { ++cnt; }
    ret = cnt >= 3;

    if (!ret) { delay(COMMAND_RETRY_INTERVAL); }
  } while (!ret && millis() - start <= COMMAND_TIMEOUT);

  return ret;
}
bool enable_analog_buttons(void) {
  bool ret = false;
  uint8_t out[sizeof(cmd_set_mode)];

  memcpy(out, cmd_set_pressures, sizeof(cmd_set_pressures));

  unsigned long start = millis();
  uint8_t cnt = 0;
  do {
    attention();
    uint8_t *in = autoShift(out, sizeof(cmd_set_pressures));
    no_attention();

    /* We can't know if we have successfully enabled analog mode until
     * we get out of config mode, so let's just be happy if we get a few
     * consecutive valid replies
     */
    if (in != NULL) { ++cnt; }
    ret = cnt >= 3;

    if (!ret) { delay(COMMAND_RETRY_INTERVAL); }
  } while (!ret && millis() - start <= COMMAND_TIMEOUT);

  return ret;
}

uint8_t get_type(void) {
  uint8_t ret = PSCTRL_UNKNOWN;

  attention();
  uint8_t *in = autoShift(cmd_type_read, 3);
  no_attention();

  if (in != NULL) {
    const uint8_t controllerType = in[3];
    if (controllerType == 0x03) {
      ret = PSCTRL_DUALSHOCK_2;
      //~ } else if (controllerType == 0x01 && in[1] == 0x42) {
      //~ return 4;		// ???
    } else if (controllerType == 0x01 && in[1] != 0x42) {
      ret = PSCTRL_GUITHERO;
    } else if (controllerType == 0x0C) {
      ret = PSCTRL_DSWIRELESS;
    }
  }

  return ret;
}

bool exit_config_mode(void) {
  bool ret = false;

  unsigned long start = millis();
  do {
    attention();
    uint8_t *in = autoShift(cmd_exit_config, 4);
    no_attention();

    ret = in != NULL && !isConfigReply(in);

    if (!ret) { delay(COMMAND_RETRY_INTERVAL); }
  } while (!ret && millis() - start <= COMMAND_TIMEOUT);

  return ret;
}

int8_t type = -1;
#define INVALID 0xFF
uint8_t dsButtons[] = {[PSB_SELECT] = XBOX_BACK,
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
uint8_t ghButtons[] = {[PSB_SELECT] = XBOX_BACK,
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

bool read(controller_t *controller) {
  bool ret = false;
  attention();
  uint8_t *in = autoShift(poll, 3);
  no_attention();

  if (in != NULL) {
    if (isConfigReply(in)) {
      // We're stuck in config mode, try to get out
      exit_config_mode();
    } else {
      // We surely have buttons
      previousButtonWord = buttonWord;
      buttonWord = ~(((PsxButtons)in[4] << 8) | in[3]);
      uint8_t *buttons = dsButtons;
      if (type == PSCTRL_GUITHERO) { buttons = ghButtons; }
      uint8_t btn;
      for (int i = 0; i < XBOX_BTN_COUNT; i++) {
        btn = buttons[i];
        if (btn != INVALID) {
          bit_write(bit_check(buttonWord, i), controller->buttons, btn);
        }
      }

      if (isDualShockReply(in) || isFlightstickReply(in)) {
        controller->l_x = (in[5] - 128) << 8;
        controller->l_y = -(in[6] - 127) << 8;
        controller->r_x = (in[7] - 128) << 8;
        controller->r_y = -(in[8] - 127) << 8;

        if (isDualShock2Reply(in)) {
          controller->lt = in[PSAB_L2 + 9];
          controller->rt = in[PSAB_R2 + 9];
          if (type == PSCTRL_GUITHERO) {
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
#define ATTN_DELAY 15
void attention(void) {
  uint8_t oldSREG = SREG;
  cli();
  *att_out &= ~att_bit;
  SPCR = spcr;
  SPSR = spsr;
  SREG = oldSREG;
  _delay_us(ATTN_DELAY);
}
void no_attention(void) {
  uint8_t oldSREG = SREG;
  cli();
  *cmd_out |= cmd_bit;
  *clk_out |= clk_bit;
  *att_out |= att_bit;
  SREG = oldSREG;
  _delay_us(ATTN_DELAY);
}
inline static uint8_t transfer(uint8_t data) {
  SPDR = data;
  /*
   * The following NOP introduces a small delay that can prevent the wait
   * loop form iterating when running at the maximum speed. This gives
   * about 10% more speed, even if it seems counter-intuitive. At lower
   * speeds it is unnoticed.
   */
  asm volatile("nop");
  while (!(SPSR & _BV(SPIF)))
    ; // wait
  return SPDR;
}
void shiftInOut(const uint8_t *out, uint8_t *in, const uint8_t len) {
  for (uint8_t i = 0; i < len; ++i) {
    uint8_t tmp = transfer(out != NULL ? out[i] : 0x5A);
    if (in != NULL) { in[i] = tmp; }

    _delay_us(INTER_CMD_BYTE_DELAY); // Very important!
  }
}
uint8_t *autoShift(const uint8_t *out, const uint8_t len) {
  uint8_t *ret = NULL;

  if (len >= 3 && len <= BUFFER_SIZE) {
    // All commands have at least 3 bytes, so shift out those first
    shiftInOut(out, inputBuffer, 3);
    if (isValidReply(inputBuffer)) {
      // Reply is good, get full length
      
      uint8_t replyLen = (inputBuffer[1] & 0x0F) * 2;

      // Shift out rest of command
      if (len > 3) { shiftInOut(out + 3, inputBuffer + 3, len - 3); }

      uint8_t left = replyLen - len + 3;
      if (left == 0) {
        // The whole reply was gathered
        ret = inputBuffer;
      } else if (len + left <= BUFFER_SIZE) {
        // Part of reply is still missing and we have space for it
        shiftInOut(NULL, inputBuffer + len, left);
        ret = inputBuffer;
      } else {
        // Reply incomplete but not enough space provided
      }
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
void ps2_cnt_tick(controller_t *controller) {
  if (type < 0) {
    if (!begin(controller)) { return; }
    // Dualshock one controllers don't have config mode
    if (enter_config_mode()) {
      spcr = spcr_ds2;
      spsr = spsr_ds2;
      type = get_type();
      enable_analog_sticks();
      enable_analog_buttons();
      exit_config_mode();
    } else {
      type = PSCTRL_DUALSHOCK_1;
    }
  }
  if (!read(controller)) { type = -1; }
}
void ps2_cnt_get_name(char *str) {
  switch (type) {
  case PSCTRL_GUITHERO:
    strcpy(str, "Guitar Hero Controller");
    break;
  case PSCTRL_DUALSHOCK_1:
    strcpy(str, "DualShock 1 Controller");
    break;
  case PSCTRL_DUALSHOCK_2:
    strcpy(str, "DualShock 2 Controller");
    break;
  case PSCTRL_DSWIRELESS:
    strcpy(str, "Wireless Sony DualShock Controller");
    break;
  case -1:
    strcpy(str, "No Controller");
    break;
  default:
    strcpy(str, "Unknown Controller");
    break;
  }
}