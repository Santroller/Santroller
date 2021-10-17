#include "lib_main.h"
static uint8_t crc5_usb_bits(const void *data, int vl, uint8_t ival) {
    const unsigned char *d = (const unsigned char *)data;
    /* This function is based on code posted by John Sullivan to Wireshark-dev
     * mailing list on Jul 21, 2019.
     *
     * "One of the properties of LFSRs is that a 1 bit in the input toggles a
     *  completely predictable set of register bits *at any point in the
     *  future*. This isn't often useful for most CRC caculations on variable
     *  sized input, as the cost of working out which those bits are vastly
     *  outweighs most other methods."
     *
     * In USB 2.0, the CRC5 is calculated on either 11 or 19 bits inputs,
     * and thus this approach is viable.
     */
    uint8_t rv = ival;
    static const uint8_t bvals[19] = {
        0x1e, 0x15, 0x03, 0x06, 0x0c, 0x18, 0x19, 0x1b,
        0x1f, 0x17, 0x07, 0x0e, 0x1c, 0x11, 0x0b, 0x16,
        0x05, 0x0a, 0x14};

    for (int i = 0; i < vl; i++) {
        uint8_t t = i & 7;
        if (bit_check(*d, t)) {
            rv ^= bvals[19 - vl + i];
        }
        if (t == 7) {
            d++;
        }
    }
    return rv;
}
uint8_t crc5_usb_11bit_input(const void *input) {
    return crc5_usb_bits(input, 11, 0x02);
}

uint8_t crc5_usb_19bit_input(const void *input) {
    return crc5_usb_bits(input, 19, 0x1d);
}

uint16_t crc_16(const void* data, size_t data_len) {
    const unsigned char* d = (const unsigned char*)data;
    unsigned int i;
    bool bit;
    unsigned char c;
    uint16_t crc = 0xffff;
    while (data_len--) {
        c = *d++;
        for (i = 0x01; i & 0xff; i <<= 1) {
            bit = crc & 0x8000;
            if (c & i) {
                bit = !bit;
            }
            crc <<= 1;
            if (bit) {
                crc ^= 0x8005;
            }
        }
        crc &= 0xffff;
    }

    uint16_t ret = crc & 0x01;
    for (i = 1; i < 16; i++) {
        crc >>= 1;
        ret = (ret << 1) | (crc & 0x01);
    }
    return ret ^ 0xffff;
}
