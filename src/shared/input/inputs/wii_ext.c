#include "wii_ext.h"
#include "config/defines.h"
#include "config/eeprom.h"
#include "i2c/i2c.h"
#include "mpu6050/inv_mpu.h"
#include "mpu6050/inv_mpu_dmp_motion_driver.h"
#include "mpu6050/mpu_math.h"
#include "util/util.h"
#include <avr/interrupt.h>
#include <avr/io.h>
#include <compat/twi.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <util/delay.h>
static const uint8_t wiiButtonBindings[16] = {
    INVALID_PIN,  INVALID_PIN,    XBOX_START,     XBOX_HOME,
    XBOX_BACK,    INVALID_PIN,    XBOX_DPAD_DOWN, XBOX_DPAD_RIGHT,
    XBOX_DPAD_UP, XBOX_DPAD_LEFT, XBOX_RB,        XBOX_Y,
    XBOX_A,       XBOX_X,         XBOX_B,         XBOX_LB};
volatile uint16_t wiiExtensionID = WII_NO_EXTENSION;

void readExtButtons(Controller_t *controller, uint16_t buttons) {
  for (uint8_t i = 0; i < sizeof(wiiButtonBindings); i++) {
    uint8_t idx = wiiButtonBindings[i];
    if (idx == INVALID_PIN) continue;
    bit_write(bit_check(buttons, i), controller->buttons, idx);
  }
}

void readDrumExt(Controller_t *controller, uint8_t *data) {
  controller->l_x = (data[0] - 0x20) << 10;
  controller->l_y = (data[1] - 0x20) << 10;
  // Mask out unused bits
  uint16_t buttons = ~(data[4] | (data[5] << 8)) & 0xfeff;
  readExtButtons(controller, buttons);
  if (config.main.subType >= MIDI_GAMEPAD && bit_check(data[3], 1)) {
    uint8_t vel = (7 - (data[3] >> 5)) << 5;
    uint8_t which = (data[2] & 0b01111100) >> 1;
    switch (which) {
    case 0x1B:
      controller->drumVelocity[XBOX_RB - 8] = vel;
      break;
    case 0x19:
      controller->drumVelocity[XBOX_B - 8] = vel;
      break;
    case 0x11:
      controller->drumVelocity[XBOX_X - 8] = vel;
      break;
    case 0x0F:
      controller->drumVelocity[XBOX_Y - 8] = vel;
      break;
    case 0x1E:
      controller->drumVelocity[XBOX_LB - 8] = vel;
      break;
    case 0x12:
      controller->drumVelocity[XBOX_A - 8] = vel;
      break;
    }
  }
  // The standard extension bindings are almost correct, but x and y are
  // swapped, so swap them
  bit_write(!bit_check(data[5], 3), controller->buttons, XBOX_X);
  bit_write(!bit_check(data[5], 5), controller->buttons, XBOX_Y);
}
void readGuitarExt(Controller_t *controller, uint8_t *data) {
  controller->l_x = ((data[0] & 0x3f) - 32) << 10;
  controller->l_y = ((data[1] & 0x3f) - 32) << 10;
  controller->r_x = -(((data[3] & 0x1f) - 16) << 10);
  uint16_t buttons = ~(data[4] | data[5] << 8);
  readExtButtons(controller, buttons);
}

void readClassicExtLowRes(Controller_t *controller, uint8_t *data) {
  uint16_t buttons;
  controller->l_x = (data[0] & 0x3f) - 32;
  controller->l_y = (data[1] & 0x3f) - 32;
  controller->r_x =
      (((data[0] & 0xc0) >> 3) | ((data[1] & 0xc0) >> 5) | (data[2] >> 7)) - 16;
  controller->r_y = (data[2] & 0x1f) - 16;
  controller->lt = ((data[3] >> 5) | ((data[2] & 0x60) >> 2));
  controller->rt = data[3] & 0x1f;
  buttons = ~(data[4] | data[5] << 8);
  readExtButtons(controller, buttons);
}
void readClassicExtHiRes(Controller_t *controller, uint8_t *data) {
  uint16_t buttons;
  controller->l_x = (data[0] - 0x80) << 8;
  controller->l_y = (data[2] - 0x80) << 8;
  controller->r_x = (data[1] - 0x80) << 8;
  controller->r_y = (data[3] - 0x80) << 8;
  controller->lt = data[4];
  controller->rt = data[5];
  buttons = ~(data[6] | (data[7] << 8));
  readExtButtons(controller, buttons);
}
void readNunchukExt(Controller_t *controller, uint8_t *data) {
  controller->l_x = (data[0] - 0x80) << 8;
  controller->l_y = (data[2] - 0x80) << 8;
  if (config.main.mapNunchukAccelToRightJoy) {
    uint16_t accX = (data[2] << 2) | ((data[5] & 0xC0) >> 6);
    uint16_t accY = (data[3] << 2) | ((data[5] & 0x30) >> 4);
    uint16_t accZ = (data[4] << 2) | ((data[5] & 0xC) >> 2);
    controller->r_x =
        atan2((float)accX - 511.0, (float)accZ - 511.0) * 180.0 / M_PI;
    controller->r_y =
        -atan2((float)accY - 511.0, (float)accZ - 511.0) * 180.0 / M_PI;
  }
  bit_write(!bit_check(data[5], 0), controller->buttons, XBOX_A);
  bit_write(!bit_check(data[5], 1), controller->buttons, XBOX_B);
}
void readDJExt(Controller_t *controller, uint8_t *data) {
  uint8_t rtt =
      (data[2] & 0x80) >> 7 | (data[1] & 0xC0) >> 5 | (data[0] & 0xC0) >> 3;

  controller->l_x = ((data[0] & 0x3F) - 0x20) << 10;
  controller->l_y = ((data[1] & 0x3F) - 0x20) << 10;
  controller->r_x =
      (data[4] & 1) ? 32 + (0x1F - (data[3] & 0x1F)) : 32 - (data[3] & 0x1F);
  controller->r_y = (data[2] & 1) ? 32 + (0x1F - rtt) : 32 - rtt;
  controller->lt = (data[3] & 0xE0) >> 5 | (data[2] & 0x60) >> 2;
  controller->rt = (data[2] & 0x1E) >> 1;
  uint16_t buttons = ~(data[4] << 8 | data[5]) & 0x63CD;
  readExtButtons(controller, buttons);
}
void readUDrawExt(Controller_t *controller, uint8_t *data) {
  controller->l_x = ((data[2] & 0x0f) << 8) | data[0];
  controller->l_y = ((data[2] & 0xf0) << 4) | data[1];
  controller->rt = data[3];
  bit_write(bit_check(data[5], 0), controller->buttons, XBOX_A);
  bit_write(bit_check(data[5], 1), controller->buttons, XBOX_B);
  bit_write(!bit_check(data[5], 2), controller->buttons, XBOX_X);
}
void readDrawsomeExt(Controller_t *controller, uint8_t *data) {
  controller->l_x = data[0] | data[1] << 8;
  controller->l_y = data[2] | data[3] << 8;
  controller->rt = data[4] | (data[5] & 0x0f) << 8;
  // controller->status = data[5]>>4;
}
void readTataconExt(Controller_t *controller, uint8_t *data) {
  uint16_t buttons = ~(data[4] << 8 | data[5]);
  readExtButtons(controller, buttons);
}
static volatile uint8_t twi_slarw;
static long lastTick = 0;
static volatile bool newData = false;

static volatile bool defaultTWI = false;
static volatile uint8_t sendIndex = READ_ID;
static volatile uint8_t dataSize = 6;
volatile bool readingMPU = false;
void initWiiExtInput(void) {
  dataSize = 6;
  sendIndex = READ_ID;
  twi_slarw = (I2C_ADDR << 1) | TW_WRITE;
  wiiExtensionID = WII_NOT_INITIALISED;
  readingMPU = false;
  defaultTWI = false;
  twi_start();
}

static uint8_t received[16];
void (*readFunction)(Controller_t *, uint8_t *) = NULL;
void tickWiiExtInput(Controller_t *controller) {
  if (newData) {
    newData = false;
    if (readFunction) { readFunction(controller, received); }
    lastTick = millis();
  }
  // if (millis() - lastTick > 1000) {}
}
void twi_reply(uint8_t ack) {
  // transmit master read ready signal, with or without ack
  if (ack) {
    TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWINT) | _BV(TWEA);
  } else {
    TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWINT);
  }
}
void twi_start(void) {
  // send start condition
  TWCR = _BV(TWINT) | _BV(TWEA) | _BV(TWEN) | _BV(TWIE) |
         _BV(TWSTA); // enable INTs
}
bool twi_stop(void) {
  // send stop condition
  TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWEA) | _BV(TWINT) | _BV(TWSTO);

  uint16_t timeoutCounter = 32767;
  while (TWCR & _BV(TWSTO)) {
    //_delay_ms( TIMEOUT_TICK );
    timeoutCounter--;
    if (timeoutCounter == 0) return false;
  }
  return true;
}
static volatile uint8_t highResCheckState = 0;
// static long lastMPU = 0;
static uint8_t receivedIndex = 0;
static uint8_t highResCheck[8];
static uint8_t orCheck = 0x00;
static uint8_t andCheck = 0xff;
// static int16_t z;
const static uint8_t packetsToSend[] = {0xf0, 0x55, 0xfb, 0x00, 0xfa, 0xfb,
                                        0x01, 0xf0, 0x55, 0xfe, 0x03, 0x00};
// #define idCnt ((readingMPU ? MPU_REG_FIFO_LEN : dataSize) - 1)
#define idCnt (dataSize - 1)
bool next = false;
ISR(TWI_vect) {
  // if (defaultTWI) {
  //   twi_tick();
  //   return;
  // }
  switch (TW_STATUS) {
  case TW_START:
  case TW_REP_START:
    dataSize = 6;
    receivedIndex = 0;
    orCheck = 0x00;
    andCheck = 0xff;
    TWDR = twi_slarw;
    twi_reply(true);
    break;

  case TW_MR_DATA_ACK: {
    uint8_t data = TWDR;
    orCheck |= data;
    andCheck &= data;
    received[receivedIndex++] = data;
  }
  case TW_MR_SLA_ACK:
    if (receivedIndex < idCnt) {
      twi_reply(true);
    } else {
      twi_reply(false);
    }
    break;
  case TW_MR_DATA_NACK: {
    uint8_t data = TWDR;
    orCheck |= data;
    andCheck &= data;
    received[receivedIndex++] = data;

    // if (readingMPU) {
    //   readingMPU = false;
    //   // for (int i = 0; i < MPU_REG_FIFO_LEN; i++) {
    //   //     Serial.print(id[i]);
    //   // }
    //   lastMPU = millis();
    //   struct s_quat q;
    //   q.w = (((long)id[0] << 24) | ((long)id[1] << 16) | ((long)id[2] << 8) |
    //          id[3]) /
    //         QUAT_SENS_FP;
    //   q.x = (((long)id[4] << 24) | ((long)id[5] << 16) | ((long)id[6] << 8) |
    //          id[7]) /
    //         QUAT_SENS_FP;
    //   q.y = (((long)id[8] << 24) | ((long)id[9] << 16) | ((long)id[10] << 8)
    //   |
    //          id[11]) /
    //         QUAT_SENS_FP;
    //   q.z = (((long)id[12] << 24) | ((long)id[13] << 16) | ((long)id[14] <<
    //   8) |
    //          id[15]) /
    //         QUAT_SENS_FP;
    //   quaternionToEuler(&q, &z, 1);
    //   // Serial.println("mpu");
    //   // Serial.println(z);
    // } else if (orCheck == 0x00 || andCheck == 0xFF) {

    if (orCheck == 0x00 || andCheck == 0xFF) {
      sendIndex = INIT_1_ID;
    } else if (sendIndex == READ_ID) {
      // asm volatile("break");
      wiiExtensionID = received[0] << 8 | received[5];
      // wiiExtensionID = receivedIndex;
      highResCheckState = 0;
      sendIndex = READ_DATA;
      switch (wiiExtensionID) {
      case WII_CLASSIC_CONTROLLER:
      case WII_CLASSIC_CONTROLLER_PRO:
        highResCheckState = 1;
        sendIndex = HIGH_RES_ID;
        break;
      case WII_GUITAR_HERO_GUITAR_CONTROLLER:
        readFunction = readGuitarExt;
        break;
      case WII_NUNCHUK:
        readFunction = readNunchukExt;
        break;
      case WII_GUITAR_HERO_DRUM_CONTROLLER:
        readFunction = readDrumExt;
        break;
      case WII_THQ_UDRAW_TABLET:
        readFunction = readUDrawExt;
        break;
      case WII_UBISOFT_DRAWSOME_TABLET:
        readFunction = readDrawsomeExt;
        sendIndex = DRAWSOME_INIT_1_ID;
        break;
      case WII_DJ_HERO_TURNTABLE:
        readFunction = readDJExt;
        break;
      case WII_TAIKO_NO_TATSUJIN_CONTROLLER:
        readFunction = readTataconExt;
        break;
      default:
        sendIndex = INIT_1_ID;
        readFunction = NULL;
      }
    } else if (sendIndex == READ_DATA) {
      if (highResCheckState) {
        if (highResCheckState == 1) {
          highResCheckState = 2;
          memcpy(highResCheck, received, sizeof(highResCheck));
        } else if (memcmp(highResCheck, received, sizeof(highResCheck)) == 0) {
          if (received[0x06] || received[0x07]) {
            dataSize = 8;
            readFunction = readClassicExtHiRes;
          } else {
            dataSize = 6;
            readFunction = readClassicExtLowRes;
          }
          highResCheckState = 0;
        } else {
          highResCheckState = 1;
        }
      } else {
        newData = true;
      }
      // if (millis() - lastMPU > 30) {
      //   lastMPU = millis();
      //   readingMPU = true;
      // }
      // Serial.println(id[0]);
    }
    // twi_slarw = ((readingMPU ? I2C_ADDR_MPU : I2C_ADDR) << 1) | TW_WRITE;
    twi_slarw = ((I2C_ADDR) << 1) | TW_WRITE;
    twi_stop();
    sei();
    _delay_us(20);
    cli();
    twi_start();
    break;
  }

  case TW_MT_SLA_ACK:
    // if (readingMPU) {
    //   TWDR = MPU_REG_FIFO;
    //   twi_reply(true);
    //   break;

    TWDR = packetsToSend[sendIndex];
    if (sendIndex != READ_ID && sendIndex != READ_DATA) {
      sendIndex++;
    } 
    twi_reply(true);
    break;
  case TW_MT_DATA_ACK:
    // if (!readingMPU) {
    if (!next) {
      if (sendIndex != READ_ID && sendIndex != READ_DATA) {
        TWDR = packetsToSend[sendIndex++];
        twi_reply(true);
        // if we were on DRAWSOME_INIT_2_DATA, then we want to skip HIGH_RES_ID
        if (sendIndex == (DRAWSOME_INIT_2_DATA + 1)) { sendIndex = READ_DATA; }
        next = true;
        return;
      } else {
        twi_slarw = ((I2C_ADDR) << 1) | TW_READ;
      }
    }
    next = false;
    // }
    // } else {
    //   twi_slarw = (I2C_ADDR_MPU << 1) | TW_READ;
    // }
    twi_stop();
    sei();
    _delay_us(175);
    cli();
    twi_start();
    break;
  }
}