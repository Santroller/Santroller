body -> neck (every 10ms)
A5 01 0A 00 00 00 00 00 00 0E 

neck -> body (every 5ms)
none
A5 01 0C 00 00 00 80 80 00 01 15 5B 

green
A5 01 0C 00 00 01 80 80 00 01 15 6C

red
A5 01 0C 00 00 02 80 80 00 01 15 35 

yellow
A5 01 0C 00 00 04 80 80 00 01 15 87 

blue
A5 01 0C 00 00 08 80 80 00 01 15 FA 

orange
A5 01 0C 00 00 10 80 80 00 01 15 00 

up
A5 01 0C 00 00 00 00 80 00 01 15 90

down
A5 01 0C 00 00 00 FF 80 00 01 15 59 

left
A5 01 0C 00 00 00 80 FF 00 01 15 69

right
A5 01 0C 00 00 00 80 00 00 01 15 82 


format:
A5 01   0C     00 00      xx       xx                 xx                 00 01 15    xx
header  len    padding    frets    dpad up/down       dpad left/right    footer      CRC-8/MAXIM-DOW

feeding in the entire packet to the following will return 0 if the packet isnt corrupt
unsigned crc8maximdow(unsigned char *data, size_t len) {
    unsigned crc = 0;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (unsigned k = 0; k < 8; k++)
            crc = crc & 1 ? (crc >> 1) ^ 0x8c : crc >> 1;
    }
    return crc;
}