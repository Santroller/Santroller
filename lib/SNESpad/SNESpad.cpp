/*
  SNESpad - Arduino/Pico library for interfacing with SNES controllers

  github.com/RobertDaleSmith/SNESpad

  Version: 2.1 - Reads the pad with a PIO state machine (Santroller), Pico SDK only
  Version: 2.0 (2023) - Extended to Pico SDK (Robert Dale Smith)
                      - Mouse and NES controller support (Robert Dale Smith)
  Version: 1.3 (11/12/2010) - get rid of shortcut constructor - seems to be broken
  Version: 1.2 (05/25/2009) - put pin numbers in constructor (Pascal Hahn)
  Version: 1.1 (09/22/2008) - fixed compilation errors in arduino 0012 (Rob Duarte)
  Version: 1.0 (09/20/2007) - Created (Rob Duarte)

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITSNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "SNESpad.h"

#include <cstring>
#include <cstdio>
#include "hardware/clocks.h"
#include "SNESpad.pio.h"

SNESpad::SNESpad(int clock, int latch, int data) {
    latchPin = latch;
    clockPin = clock;
    dataPin = data;
}

SNESpad::~SNESpad() {
    end();
}

void SNESpad::begin() {
    if (started) {
        return;
    }
    started = true;
    initGpio();
    if (!pio_claim_free_sm_and_add_program(&snes_pad_program, &pio, &sm, &offset)) {
        // no free state machine, so poll() bit-bangs instead
        pio = nullptr;
#if SNES_PAD_DEBUG==true
        printf("SNESpad::begin no free PIO, bit-banging\n");
#endif
        return;
    }
    pio_gpio_init(pio, clockPin);
    pio_gpio_init(pio, latchPin);
    pio_sm_set_consecutive_pindirs(pio, sm, clockPin, 1, true);
    pio_sm_set_consecutive_pindirs(pio, sm, latchPin, 1, true);
    pio_sm_set_consecutive_pindirs(pio, sm, dataPin, 1, false);
    pio_sm_config c = snes_pad_program_get_default_config(offset);
    sm_config_set_sideset_pins(&c, clockPin);
    sm_config_set_set_pins(&c, latchPin, 1);
    sm_config_set_in_pins(&c, dataPin);
    // first bit ends up in bit 0
    sm_config_set_in_shift(&c, true, true, 32);
    sm_config_set_clkdiv(&c, clock_get_hz(clk_sys) / 1000000.0f);
    pio_sm_init(pio, sm, offset, &c);
    pio_sm_set_enabled(pio, sm, true);
#if SNES_PAD_DEBUG==true
    printf("SNESpad::begin\n");
#endif
}

void SNESpad::end() {
    if (!started) {
        return;
    }
    started = false;
    if (pio) {
        pio_sm_set_enabled(pio, sm, false);
        pio_remove_program_and_unclaim_sm(&snes_pad_program, pio, sm, offset);
        pio = nullptr;
    }
    gpio_init(clockPin);
    gpio_init(latchPin);
    gpio_init(dataPin);
    decode(0);
}

void SNESpad::poll() {
    if (!started) {
        return;
    }
    if (!pio) {
        decode(read());
        return;
    }
    // The state machine reads on its own, so only the newest read matters
    bool got = false;
    uint32_t raw = 0;
    while (!pio_sm_is_rx_fifo_empty(pio, sm)) {
        raw = pio_sm_get(pio, sm);
        got = true;
    }
    if (!got) {
        return;
    }
    decode(~raw);
    if (wantsSpeedChange() && pio_sm_is_tx_fifo_empty(pio, sm)) {
        // pulses clock during the next latch
        pio_sm_put(pio, sm, 1);
    }
}

void SNESpad::decode(uint32_t state) {
    int8_t lastType = type;
    // A connected pad, NES pad or mouse always drives some bits low (active), an empty
    // port is pulled up and reads as all high
    if (!state) {
        type = SNES_PAD_NONE;
        mouseSpeedFails = 0;
        mouseSpeed = 0;
    } else {
        // check device type id
        bool isSNES = ((state & SNES_DEVICE_ID) >> 12) == SNES_PAD_ID;
        bool isMouse = ((state & SNES_DEVICE_ID) >> 12) == SNES_MOUSE_ID;
        bool isNES = ((state >> 8) & 0b11111111) == 0b11111111;

        if (isSNES) {
            type = SNES_PAD_BASIC;
        } else if (isNES) {
            type = SNES_PAD_NES;
        } else if (isMouse) {
            uint8_t lastMouseSpeed = mouseSpeed;

            // parse mouse speed bits
            mouseSpeed = ((state & SNES_MOUSE_SPEED) >> 10);
            if (mouseSpeed > 2) mouseSpeed = 0;

            // detect hyperkin mouse failure to change speed to halt further attempts
            if (
                mouseSpeed != SNES_MOUSE_FAST
                && lastMouseSpeed == mouseSpeed
                && mouseSpeedFails < SNES_MOUSE_THRESHOLD
            ) {
                mouseSpeedFails++;
            }

            type = SNES_PAD_MOUSE;
        } else {
            type = SNES_PAD_NONE; //UNKNOWN device id
        }
    }

    mouseX          = 0;
    mouseY          = 0;
    buttonA         = 0;
    buttonB         = 0;
    buttonX         = 0;
    buttonY         = 0;
    buttonStart     = 0;
    buttonSelect    = 0;
    buttonL         = 0;
    buttonR         = 0;
    directionUp     = 0;
    directionDown   = 0;
    directionLeft   = 0;
    directionRight  = 0;

    switch (type) {
        case SNES_PAD_BASIC:
            directionLeft =  (state & SNES_LEFT);
            directionUp =    (state & SNES_UP);
            directionRight = (state & SNES_RIGHT);
            directionDown =  (state & SNES_DOWN);

            buttonSelect =   (state & SNES_SELECT);
            buttonStart =    (state & SNES_START);
            buttonB =        (state & SNES_B);
            buttonY =        (state & SNES_Y);
            buttonA =        (state & SNES_A);
            buttonX =        (state & SNES_X);
            buttonL =        (state & SNES_L);
            buttonR =        (state & SNES_R);

            break;
        case SNES_PAD_NES:
            directionLeft =  (state & SNES_LEFT);
            directionUp =    (state & SNES_UP);
            directionRight = (state & SNES_RIGHT);
            directionDown =  (state & SNES_DOWN);

            buttonSelect =   (state & SNES_SELECT);
            buttonStart =    (state & SNES_START);
            buttonB =        (state & SNES_Y);
            buttonA =        (state & SNES_B);

            break;
        case SNES_PAD_MOUSE:
        {
            int x = 127;  //set center position [0-255]
            int y = 127;

            // Mouse X axis
            x = (state & SNES_MOUSE_X) >> 25;
            x = reverse(x) * SNES_MOUSE_PRECISION;
            if (state & SNES_MOUSE_X_SIGN) x = 127 - x;
            else x = 127 + x;

            // Mouse Y axis
            y = (state & SNES_MOUSE_Y) >> 17;
            y = reverse(y) * SNES_MOUSE_PRECISION;
            if (state & SNES_MOUSE_Y_SIGN) y = 127 - y;
            else y = 127 + y;

            mouseX  = x;
            mouseY  = y;
            buttonB = (state & SNES_X);
            buttonA = (state & SNES_A);

            break;
        }
    }

#if SNES_PAD_DEBUG==true
    if (type != lastType) {
        printf("Device Type: %d\n", type);
    }
    if (_lastRead != state) {
        printf(
            "A=%1d B=%1d X=%1d Y=%1d L=%1d R=%1d Select=%1d Start=%1d Mouse X=%4d Y=%4d\n",
            buttonA, buttonB, buttonX, buttonY, buttonL, buttonR, buttonSelect, buttonStart, mouseX, mouseY
        );
    }
    _lastRead = state;
#else
    (void)lastType;
#endif
}

bool SNESpad::wantsSpeedChange() const {
    // default mouse to fastest speed
    return type == SNES_PAD_MOUSE
        && mouseSpeed != SNES_MOUSE_FAST
        && mouseSpeedFails < SNES_MOUSE_THRESHOLD;
}

// init gpio pins
void SNESpad::initGpio() {
    gpio_init(clockPin);
    gpio_init(latchPin);
    gpio_init(dataPin);

    gpio_put(clockPin, 1);
    gpio_put(latchPin, 0);
    gpio_set_dir(clockPin, GPIO_OUT);
    gpio_set_dir(latchPin, GPIO_OUT);
    gpio_set_dir(dataPin, GPIO_IN);
    gpio_pull_up(dataPin);
}

// signal mouse to go to next speed if not at desired speed
void SNESpad::speed()
{
    if (wantsSpeedChange()) {
        gpio_put(clockPin,0);
        busy_wait_us(6);

        gpio_put(clockPin,1);
        busy_wait_us(12);
    }
}

// clock in a data bit
uint32_t SNESpad::clock()
{
    uint32_t ret = 0;

    gpio_put(clockPin,0);
    busy_wait_us(6);

    ret = gpio_get(dataPin);
    gpio_put(clockPin,1);

    busy_wait_us(6);
    return ret;
}

// latch to start read
void SNESpad::latch()
{
    gpio_put(latchPin,1);
    busy_wait_us(12);

    speed(); // check/set mouse speed

    gpio_put(latchPin,0);
    busy_wait_us(6);
}

// Bit-banged read, for when no PIO state machine is free. Returns the decoded format
// (active high), or 0 when nothing is plugged in.
uint32_t SNESpad::read()
{
    uint32_t ret = 0;
    uint8_t i;

    /* A connected device will pull the data line low prior to latch.
       A disconnected pin is kept high by internal pull_up.*/
    uint32_t disconnected = gpio_get(dataPin);

    latch();
    for (i = 0; i < 32; i++) {
        uint32_t bit = clock(); // clock shift bit in
        ret |= bit << i;

        if (i == 15) {
            bool read_extra = !bit; // check if mouse
            if (!read_extra) break; // skip extra bytes if not
            busy_wait_us(12);
        }
    }
    ret = ~ret; // buttons are active low, so invert bits

    // verify controller or mouse is connected
    if (disconnected && !(ret & 0xFFFF)) {
        return 0;
    }
    return ret;
}

// reverse bits within a byte (ex: 0b1000 -> 0b0001)
uint8_t SNESpad::reverse(uint8_t c) {
    char r = 0;
    for(int i = 0; i < 8; i++) {
        r <<= 1;
        r |= c & 1;
        c >>= 1;
    }
    return r;
}
