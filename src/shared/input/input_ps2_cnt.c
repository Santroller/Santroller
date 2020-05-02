#include "input_ps2_cnt.h"
#include "../util.h"
#include "Arduino.h"
#include <avr/io.h>
#include <util/delay.h>
#include <math.h>
#include <stdio.h>


static uint8_t enter_config[] = {0x01, 0x43, 0x00, 0x01, 0x00};
static uint8_t set_mode[] = {0x01, 0x44, 0x00, 0x01, 0x03,
                             0x00, 0x00, 0x00, 0x00};
static uint8_t set_uint8_ts_large[] = {0x01, 0x4F, 0x00, 0xFF, 0xFF,
                                       0x03, 0x00, 0x00, 0x00};
static uint8_t exit_config[] = {0x01, 0x43, 0x00, 0x00, 0x5A,
                                0x5A, 0x5A, 0x5A, 0x5A};
static uint8_t enable_rumble[] = {0x01, 0x4D, 0x00, 0x00, 0x01};
static uint8_t type_read[] = {0x01, 0x45, 0x00, 0x5A, 0x5A,
                              0x5A, 0x5A, 0x5A, 0x5A};

void CLK_SET(void) {
  register uint8_t old_sreg = SREG;
  cli();
  *_clk_oreg |= _clk_mask;
  SREG = old_sreg;
}

void CLK_CLR(void) {
  register uint8_t old_sreg = SREG;
  cli();
  *_clk_oreg &= ~_clk_mask;
  SREG = old_sreg;
}

void CMD_SET(void) {
  register uint8_t old_sreg = SREG;
  cli();
  *_cmd_oreg |= _cmd_mask; // SET(*_cmd_oreg,_cmd_mask);
  SREG = old_sreg;
}

void CMD_CLR(void) {
  register uint8_t old_sreg = SREG;
  cli();
  *_cmd_oreg &= ~_cmd_mask; // SET(*_cmd_oreg,_cmd_mask);
  SREG = old_sreg;
}

void ATT_SET(void) {
  register uint8_t old_sreg = SREG;
  cli();
  *_att_oreg |= _att_mask;
  SREG = old_sreg;
}

void ATT_CLR(void) {
  register uint8_t old_sreg = SREG;
  cli();
  *_att_oreg &= ~_att_mask;
  SREG = old_sreg;
}

bool DAT_CHK(void) { return (*_dat_ireg & _dat_mask) ? true : false; }
// /****************************************************************************************/
// bool NewButtonState() { return ((last_buttons ^ buttons) > 0); }

// /****************************************************************************************/
// bool NewButtonState(unsigned int button) {
//   return (((last_buttons ^ buttons) & button) > 0);
// }

// /****************************************************************************************/
// bool ButtonPressed(unsigned int button) {
//   return (NewButtonState(button) & Button(button));
// }

// /****************************************************************************************/
// bool ButtonReleased(unsigned int button) {
//   return ((NewButtonState(button)) & ((~last_buttons & button) > 0));
// }

// /****************************************************************************************/
// bool Button(uint16_t button) { return ((~buttons & button) > 0); }

// /****************************************************************************************/
// unsigned int ButtonDatauint8_t() { return (~buttons); }

// /****************************************************************************************/
// uint8_t Analog(uint8_t button) { return PS2data[button]; }

/****************************************************************************************/
unsigned char _gamepad_shiftinout(char uint8_t) {
  unsigned char tmp = 0;
  for (unsigned char i = 0; i < 8; i++) {
    if (CHK(uint8_t, i))
      CMD_SET();
    else
      CMD_CLR();

    CLK_CLR();
    _delay_us(CTRL_CLK);

    // if(DAT_CHK()) SET(tmp,i);
    if (DAT_CHK()) bit_set(tmp, i);

    CLK_SET();
#if CTRL_CLK_HIGH
    _delay_us(CTRL_CLK_HIGH);
#endif
  }
  CMD_SET();
  _delay_us(CTRL_uint8_t_DELAY);
  return tmp;
}

/****************************************************************************************/
bool read_gamepad(bool motor1, uint8_t motor2) {
  double temp = millis() - last_read;

  if (temp > 1500) // waited to long
    reconfig_gamepad();

  if (temp < read_delay) // waited too short
    delay(read_delay - temp);

  uint8_t dword[9] = {0x01, 0x42, 0, motor1, motor2, 0, 0, 0, 0};
  uint8_t dword2[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  // Try a few times to get valid data...
  for (uint8_t RetryCnt = 0; RetryCnt < 5; RetryCnt++) {
    CMD_SET();
    CLK_SET();
    ATT_CLR(); // low enable joystick

    _delay_us(CTRL_uint8_t_DELAY);
    // Send the command to send button and joystick data;
    for (int i = 0; i < 9; i++) { PS2data[i] = _gamepad_shiftinout(dword[i]); }

    if (PS2data[1] == 0x79) { // if controller is in full data return mode, get
                              // the rest of data
      for (int i = 0; i < 12; i++) {
        PS2data[i + 9] = _gamepad_shiftinout(dword2[i]);
      }
    }

    ATT_SET(); // HI disable joystick
    // Check to see if we received valid data or not.
    // We should be in analog mode for our data to be valid (analog == 0x7_)
    if ((PS2data[1] & 0xf0) == 0x70) break;

    // If we got to here, we are not in analog mode, try to recover...
    reconfig_gamepad(); // try to get back into Analog mode.
    delay(read_delay);
  }

  // If we get here and still not in analog mode (=0x7_), try increasing the
  // read_delay...
  if ((PS2data[1] & 0xf0) != 0x70) {
    if (read_delay < 10) read_delay++; // see if this helps out...
  }

#ifdef PS2X_COM_DEBUG
  Serial.print("OUT:IN ");
  for (int i = 0; i < 9; i++) {
    Serial.print(dword[i], HEX);
    Serial.print(":");
    Serial.print(PS2data[i], HEX);
    Serial.print(" ");
  }
  for (int i = 0; i < 12; i++) {
    Serial.print(dword2[i], HEX);
    Serial.print(":");
    Serial.print(PS2data[i + 9], HEX);
    Serial.print(" ");
  }
  Serial.println("");
#endif

  last_buttons = buttons; // store the previous buttons states

#if defined(__AVR__)
  buttons =
      *(uint16_t *)(PS2data + 3); // store as one value for multiple functions
#else
  buttons = (uint16_t)(PS2data[4] << 8) +
            PS2data[3]; // store as one value for multiple functions
#endif
  last_read = millis();
  return ((PS2data[1] & 0xf0) == 0x70); // 1 = OK = analog mode - 0 = NOK
}

/****************************************************************************************/
uint8_t config_gamepad(uint8_t clk, uint8_t cmd, uint8_t att, uint8_t dat,
                       bool pressures, bool rumble) {

  uint8_t temp[sizeof(type_read)];

  _clk_mask = digitalPinToBitMask(clk);
  _clk_oreg = portOutputRegister(digitalPinToPort(clk));
  _cmd_mask = digitalPinToBitMask(cmd);
  _cmd_oreg = portOutputRegister(digitalPinToPort(cmd));
  _att_mask = digitalPinToBitMask(att);
  _att_oreg = portOutputRegister(digitalPinToPort(att));
  _dat_mask = digitalPinToBitMask(dat);
  _dat_ireg = portInputRegister(digitalPinToPort(dat));

  pinMode(clk, OUTPUT); // configure ports
  pinMode(att, OUTPUT);
  pinMode(cmd, OUTPUT);
  pinMode(dat, INPUT_PULLUP);

  CMD_SET(); // SET(*_cmd_oreg,_cmd_mask);
  CLK_SET();

  // new error checking. First, read gamepad a few times to see if it's talking
  read_gamepad(false, 0x00);
  read_gamepad(false, 0x00);

  // see if it talked - see if mode came back.
  // If still anything but 41, 73 or 79, then it's not talking
  if (PS2data[1] != 0x41 && PS2data[1] != 0x42 && PS2data[1] != 0x73 &&
      PS2data[1] != 0x79) {
#ifdef PS2X_DEBUG
    Serial.println("Controller mode not matched or no controller found");
    Serial.print("Expected 0x41, 0x42, 0x73 or 0x79, but got ");
    Serial.println(PS2data[1], HEX);
#endif
    return 1; // return error code 1
  }

  // try setting mode, increasing delays if need be.
  read_delay = 1;

  for (int y = 0; y <= 10; y++) {
    sendCommandString(enter_config, sizeof(enter_config)); // start config run

    // read type
    _delay_us(CTRL_uint8_t_DELAY);

    CMD_SET();
    CLK_SET();
    ATT_CLR(); // low enable joystick

    _delay_us(CTRL_uint8_t_DELAY);

    for (int i = 0; i < 9; i++) { temp[i] = _gamepad_shiftinout(type_read[i]); }

    ATT_SET(); // HI disable joystick

    controller_type = temp[3];

    sendCommandString(set_mode, sizeof(set_mode));
    if (rumble) {
      sendCommandString(enable_rumble, sizeof(enable_rumble));
      en_Rumble = true;
    }
    if (pressures) {
      sendCommandString(set_uint8_ts_large, sizeof(set_uint8_ts_large));
      en_Pressures = true;
    }
    sendCommandString(exit_config, sizeof(exit_config));

    read_gamepad(false, 0x00);

    if (pressures) {
      if (PS2data[1] == 0x79) break;
      if (PS2data[1] == 0x73) return 3;
    }

    if (PS2data[1] == 0x73) break;

    if (y == 10) {
#ifdef PS2X_DEBUG
      Serial.println("Controller not accepting commands");
      Serial.print("mode still set at");
      Serial.println(PS2data[1], HEX);
#endif
      return 2; // exit function with error
    }
    read_delay += 1; // add 1ms to read_delay
  }
  return 0; // no error if here
}

/****************************************************************************************/
void sendCommandString(uint8_t string[], uint8_t len) {
#ifdef PS2X_COM_DEBUG
  uint8_t temp[len];
  ATT_CLR(); // low enable joystick
  _delay_us(CTRL_uint8_t_DELAY);

  for (int y = 0; y < len; y++) temp[y] = _gamepad_shiftinout(string[y]);

  ATT_SET();         // high disable joystick
  delay(read_delay); // wait a few

  Serial.println("OUT:IN Configure");
  for (int i = 0; i < len; i++) {
    Serial.print(string[i], HEX);
    Serial.print(":");
    Serial.print(temp[i], HEX);
    Serial.print(" ");
  }
  Serial.println("");
#else
  ATT_CLR();            // low enable joystick
  _delay_us(CTRL_uint8_t_DELAY);
  for (int y = 0; y < len; y++) _gamepad_shiftinout(string[y]);
  ATT_SET();         // high disable joystick
  delay(read_delay); // wait a few
#endif
}

/****************************************************************************************/
uint8_t readType() {
  if (controller_type == 0x03)
    return 1;
  else if (controller_type == 0x01 && PS2data[1] == 0x42)
    return 4;
  else if (controller_type == 0x01 && PS2data[1] != 0x42)
    return 2;
  else if (controller_type == 0x0C)
    return 3; // 2.4G Wireless Dual Shock PS2 Game Controller

  return 0;
}

/****************************************************************************************/
void enableRumble() {
  sendCommandString(enter_config, sizeof(enter_config));
  sendCommandString(enable_rumble, sizeof(enable_rumble));
  sendCommandString(exit_config, sizeof(exit_config));
  en_Rumble = true;
}

/****************************************************************************************/
bool enablePressures() {
  sendCommandString(enter_config, sizeof(enter_config));
  sendCommandString(set_uint8_ts_large, sizeof(set_uint8_ts_large));
  sendCommandString(exit_config, sizeof(exit_config));

  read_gamepad(false, 0x00);
  read_gamepad(false, 0x00);

  if (PS2data[1] != 0x79) return false;

  en_Pressures = true;
  return true;
}

/****************************************************************************************/
void reconfig_gamepad() {
  sendCommandString(enter_config, sizeof(enter_config));
  sendCommandString(set_mode, sizeof(set_mode));
  if (en_Rumble) sendCommandString(enable_rumble, sizeof(enable_rumble));
  if (en_Pressures)
    sendCommandString(set_uint8_ts_large, sizeof(set_uint8_ts_large));
  sendCommandString(exit_config, sizeof(exit_config));
}

/****************************************************************************************/

uint8_t type = -1;
// Obvs these need to be made configurable!
#define PS2_DAT 13 // 14
#define PS2_CMD 11 // 15
#define PS2_SEL 10 // 16
#define PS2_CLK 12 // 17
void ps2_cnt_tick(controller_t *controller) {
  if (type == -1) {
    if (!config_gamepad(PS2_CLK, PS2_CMD, PS2_SEL, PS2_DAT, false, false)) {
      return;
    }
    type = readType();
  }
  if (read_gamepad(false, 0x00)) {
    bit_write(bit_check(buttons, PSB_CROSS), controller->buttons, XBOX_A);
  }
}
void ps2_cnt_get_name(char *str) {
  switch (type) {
  case GUITAR_HERO:
    strcpy(str, "Guitar Hero Controller");
    break;
  case DUAL_SHOCK:
    strcpy(str, "DualShock Controller");
    break;
  case WIRELESS_DUAL_SHOCK:
    strcpy(str, "Wireless Sony DualShock Controller");
    break;
  default:
    strcpy(str, "Unknown Controler");
    break;
  }
}