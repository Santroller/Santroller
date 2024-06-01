
#include "wii.h"

#include <Wire.h>
#include <string.h>

#include "Arduino.h"
#include "config.h"
#include "controllers.h"
#include "io.h"
#include "wm_crypto.h"
#include "shared_main.h"
static volatile unsigned char wm_rand[10];
static volatile unsigned char wm_key[6];
static volatile unsigned char wm_ft[8];
static volatile unsigned char wm_sb[8];
// calibration data
const unsigned char cal_data[32] = {
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00
};
#if DEVICE_TYPE_IS_GUITAR
const unsigned char id[6] = {0x00, 0x00, 0xA4, 0x20, 0x01, 0x03};
#elif DEVICE_TYPE_IS_DRUM
const unsigned char id[6] = {0x01, 0x00, 0xA4, 0x20, 0x01, 0x03};
#elif DEVICE_TYPE == DJ_HERO_TURNTABLE
const unsigned char id[6] = {0x03, 0x00, 0xA4, 0x20, 0x01, 0x03};
#else
const unsigned char id[6] = {0x01, 0x00, 0xA4, 0x20, 0x01, 0x01};
#endif

// virtual register
static unsigned char twi_reg[256];

void initWiiOutput() {
    memset(twi_reg, 0, sizeof(twi_reg));
    twi_reg[0xF0] = 0; // disable encryption

	// set id
	for(unsigned int i = 0, j = 0xFA; i < 6; i++, j++)
	{
		twi_reg[j] = id[i];
	}

	// set calibration data
	for(unsigned int i = 0, j = 0x20; i < 6; i++, j++)
	{
		twi_reg[j] = cal_data[i];
	}
}
void setInputs(uint8_t* inputs, uint8_t len) {
    memcpy(twi_reg,inputs,len);
}
/*

I'd like to thank Hector Martin for posting his encryption method!
His website is http://www.marcansoft.com/
Decryption method found at https://web.archive.org/web/20131106235350/http://www.derkeiler.com/pdf/Newsgroups/sci.crypt/2008-11/msg00110.pdf

*/

unsigned char wm_ror8(unsigned char a, unsigned char b) {
    // bit shift with roll-over
    return (a >> b) | ((a << (8 - b)) & 0xFF);
}

void wm_gentabs() {
    unsigned char idx;

    // check all idx
    for (idx = 0; idx < 7; idx++) {
        // generate test key
        unsigned char ans[6];
        unsigned char tkey[6];
        unsigned char t0[10];

        for (unsigned char i = 0; i < 6; i++) {
            ans[i] = pgm_read_byte(&(ans_tbl[idx][i]));
        }
        for (unsigned char i = 0; i < 10; i++) {
            t0[i] = pgm_read_byte(&(sboxes[0][wm_rand[i]]));
        }

        tkey[0] = ((wm_ror8((ans[0] ^ t0[5]), (t0[2] % 8)) - t0[9]) ^ t0[4]);
        tkey[1] = ((wm_ror8((ans[1] ^ t0[1]), (t0[0] % 8)) - t0[5]) ^ t0[7]);
        tkey[2] = ((wm_ror8((ans[2] ^ t0[6]), (t0[8] % 8)) - t0[2]) ^ t0[0]);
        tkey[3] = ((wm_ror8((ans[3] ^ t0[4]), (t0[7] % 8)) - t0[3]) ^ t0[2]);
        tkey[4] = ((wm_ror8((ans[4] ^ t0[1]), (t0[6] % 8)) - t0[3]) ^ t0[4]);
        tkey[5] = ((wm_ror8((ans[5] ^ t0[7]), (t0[8] % 8)) - t0[5]) ^ t0[9]);

        // compare with actual key
        bool found = true;
        for (unsigned char i = 0; i < 6; i++) {
           if (tkey[i] != wm_key[i]) {
              found = false;
              break;
           }
        }
        if (found) break;  // if match, then use this idx
    }

    // generate encryption from idx key and rand
    wm_ft[0] = pgm_read_byte(&(sboxes[idx + 1][wm_key[4]])) ^ pgm_read_byte(&(sboxes[idx + 2][wm_rand[3]]));
    wm_ft[1] = pgm_read_byte(&(sboxes[idx + 1][wm_key[2]])) ^ pgm_read_byte(&(sboxes[idx + 2][wm_rand[5]]));
    wm_ft[2] = pgm_read_byte(&(sboxes[idx + 1][wm_key[5]])) ^ pgm_read_byte(&(sboxes[idx + 2][wm_rand[7]]));
    wm_ft[3] = pgm_read_byte(&(sboxes[idx + 1][wm_key[0]])) ^ pgm_read_byte(&(sboxes[idx + 2][wm_rand[2]]));
    wm_ft[4] = pgm_read_byte(&(sboxes[idx + 1][wm_key[1]])) ^ pgm_read_byte(&(sboxes[idx + 2][wm_rand[4]]));
    wm_ft[5] = pgm_read_byte(&(sboxes[idx + 1][wm_key[3]])) ^ pgm_read_byte(&(sboxes[idx + 2][wm_rand[9]]));
    wm_ft[6] = pgm_read_byte(&(sboxes[idx + 1][wm_rand[0]])) ^ pgm_read_byte(&(sboxes[idx + 2][wm_rand[6]]));
    wm_ft[7] = pgm_read_byte(&(sboxes[idx + 1][wm_rand[1]])) ^ pgm_read_byte(&(sboxes[idx + 2][wm_rand[8]]));

    wm_sb[0] = pgm_read_byte(&(sboxes[idx + 1][wm_key[0]])) ^ pgm_read_byte(&(sboxes[idx + 2][wm_rand[1]]));
    wm_sb[1] = pgm_read_byte(&(sboxes[idx + 1][wm_key[5]])) ^ pgm_read_byte(&(sboxes[idx + 2][wm_rand[4]]));
    wm_sb[2] = pgm_read_byte(&(sboxes[idx + 1][wm_key[3]])) ^ pgm_read_byte(&(sboxes[idx + 2][wm_rand[0]]));
    wm_sb[3] = pgm_read_byte(&(sboxes[idx + 1][wm_key[2]])) ^ pgm_read_byte(&(sboxes[idx + 2][wm_rand[9]]));
    wm_sb[4] = pgm_read_byte(&(sboxes[idx + 1][wm_key[4]])) ^ pgm_read_byte(&(sboxes[idx + 2][wm_rand[7]]));
    wm_sb[5] = pgm_read_byte(&(sboxes[idx + 1][wm_key[1]])) ^ pgm_read_byte(&(sboxes[idx + 2][wm_rand[8]]));
    wm_sb[6] = pgm_read_byte(&(sboxes[idx + 1][wm_rand[3]])) ^ pgm_read_byte(&(sboxes[idx + 2][wm_rand[5]]));
    wm_sb[7] = pgm_read_byte(&(sboxes[idx + 1][wm_rand[2]])) ^ pgm_read_byte(&(sboxes[idx + 2][wm_rand[6]]));
}
void recv_data(uint8_t addr, uint8_t data) {
    if (twi_reg[0xF0] == 0xAA && addr != 0xF0)  // if encryption is on
    {
        // decrypt
        twi_reg[addr] = (data ^ wm_sb[addr % 8]) + wm_ft[addr % 8];
    } else {
        twi_reg[addr] = data;
    }
}
void recv_end(uint8_t addr, uint8_t len) {
    if (addr >= 0x40 && addr < 0x46) {
        for (unsigned int i = 0; i < 6; i++) {
            wm_rand[9 - i] = twi_reg[0x40 + i];
        }
    } else if (addr >= 0x46 && addr < 0x4C) {
        for (unsigned int i = 6; i < 10; i++) {
            wm_rand[9 - i] = twi_reg[0x40 + i];
        }
        for (unsigned int i = 0; i < 2; i++) {
            wm_key[5 - i] = twi_reg[0x40 + 10 + i];
        }
    } else if (addr >= 0x4C && addr < 0x50) {
        for (unsigned int i = 2; i < 6; i++) {
            wm_key[5 - i] = twi_reg[0x40 + 10 + i];
        }
        if (addr + len == 0x50) {
            // generate decryption once all data is loaded
            wm_gentabs();
        }
    }
}

// Called when the I2C slave is read from
uint8_t req_data(uint8_t addr) {
    if (twi_reg[0xF0] == 0xAA)  // encryption is on
    {
        // encrypt
        return (twi_reg[addr] - wm_ft[addr % 8]) ^ wm_sb[addr % 8];
    } else {
        return twi_reg[addr];
    }
}