/*
  twi.c - TWI/I2C library for Wiring & Arduino
  Copyright (c) 2006 Nicholas Zambetti.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

  Modified 2012 by Todd Krein (todd@krein.org) to implement repeated starts
  Modified 2019 by IanSC to return after a timeout period
*/

#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

// === MODIFIED ===
#include "hardware/i2c.h"
#include "i2c/i2c.h"
#include "pins/pins.h"
#include "pins_arduino.h"
#include "timer/timer.h"
#include "util/util.h"

// === MODIFIED ===
static uint16_t TIMEOUT = 1000;

/*
 * Function twi_init
 * Desc     readys twi pins and sets twi bitrate
 * Input    none
 * Output   none
 */
void twi_init(void) {
  i2c_init(i2c0, TWI_FREQ);
  i2c_set_baudrate(i2c0, TWI_FREQ);
  gpio_set_function(PIN_WIRE_SDA, GPIO_FUNC_I2C);
  gpio_set_function(PIN_WIRE_SCL, GPIO_FUNC_I2C);
  gpio_pull_up(PIN_WIRE_SDA);
  gpio_pull_up(PIN_WIRE_SCL);
}

/*
 * Function twi_disable
 * Desc     disables twi pins
 * Input    none
 * Output   none
 */
void twi_disable(void) { i2c_deinit(i2c0); }

/*
 * Function twi_readFrom
 * Desc     attempts to become twi bus master and read a
 *          series of bytes from a device on the bus
 * Input    address: 7bit i2c device address
 *          data: pointer to byte array
 *          length: number of bytes to read into array
 *          sendStop: Boolean indicating whether to send a stop at the end
 * Output   number of bytes read
 */
// === MODIFIED ===
bool twi_readFrom(uint8_t address, uint8_t *data, uint8_t length,
                  uint8_t sendStop) {
  return i2c_read_blocking(i2c0, address, data, length, !sendStop) > 0;
}
// The RPI Pico is stupid. We have to bitbang I2C when writing packets that are <2 in length (which is like most of what we write...)
#define DELAY (500000 / TWI_FREQ) + 1
void low(uint8_t pin) { gpio_set_dir(pin, GPIO_OUT); }
void release(uint8_t pin) { gpio_set_dir(pin, GPIO_IN); }
static inline void delayTWI() {
  busy_wait_us_32(DELAY);
}
// TODO: any reason why this is needed? waiting 7us is a long time in the grand scheme of things (especially since this is done twice per bit, and there are 16 bits to write. thats 112 us just on bits)
static inline void delayTWI2() {
  busy_wait_us_32(DELAY + 5);
}
void sclRelease() {
  uint32_t count = 255;
  release(PIN_WIRE_SCL);
  delayTWI2();
  // For clock stretching, wait for the SCL pin to be released, with timeout.
  for (; gpio_get(PIN_WIRE_SCL) == 0 && count; --count) {
    busy_wait_us_32(1);
  }
}
void sclLow() { low(PIN_WIRE_SCL); }
void sdaLow() {
  low(PIN_WIRE_SDA);
}
void sdaRelease() {
  release(PIN_WIRE_SDA);
}
void stop() {
  delayTWI();
  sdaLow();
  delayTWI();
  sclRelease();
  sdaRelease();
  delayTWI();
}
void start() {
  sdaRelease();
  delayTWI();
  sclRelease();
  sdaLow();
  delayTWI();
}
bool writeByte(uint8_t val) {
  delayTWI2();
  sclLow();
  for (int i = 7; i >= 0; i--) {
    if ((val >> i) & 1) {
      sdaRelease();
    } else {
      sdaLow();
    }
    delayTWI2();
    sclRelease();
    sdaRelease();
    sclLow();
  }
  sdaRelease();
  delayTWI();
  sclRelease();
  bool ack = gpio_get(PIN_WIRE_SDA);
  delayTWI();
  sclLow();
  return ack;
}
/*
 * Function twi_writeTo
 * Desc     attempts to become twi bus master and write a
 *          series of bytes to a device on the bus
 * Input    address: 7bit i2c device address
 *          data: pointer to byte array
 *          length: number of bytes in array
 *          wait: boolean indicating to wait for write or not
 *          sendStop: boolean indicating whether or not to send a stop at the
 * end Output   0 .. success 1 .. length too long for buffer 2 .. address send,
 * NACK received 3 .. data send, NACK received 4 .. other twi error (lost bus
 * arbitration, bus error, ..)
 */
bool twi_writeTo(uint8_t address, uint8_t *data, uint8_t length, uint8_t wait,
                 uint8_t sendStop) {
  // Hardware twi cant handle writes that are small
  if (length <= 2) {
    // Set gpio so we can bitbang
    gpio_set_function(PIN_WIRE_SCL, GPIO_FUNC_SIO);
    release(PIN_WIRE_SCL);
    gpio_put(PIN_WIRE_SCL, 0);
    gpio_set_function(PIN_WIRE_SDA, GPIO_FUNC_SIO);
    release(PIN_WIRE_SDA);
    gpio_put(PIN_WIRE_SDA, 0);
    delayTWI();
    // Send start
    start();
    writeByte(address << 1);
    while (length) {
      writeByte(*data);
      data++;
      length--;
    }
    // Send stop
    stop();

    // And now enable hardware twi again
    gpio_set_function(PIN_WIRE_SDA, GPIO_FUNC_I2C);
    gpio_set_function(PIN_WIRE_SCL, GPIO_FUNC_I2C);
    return true;
  }
  return i2c_write_blocking(i2c0, address, data, length, !sendStop) > 0;
}
