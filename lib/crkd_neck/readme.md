# CRKD Neck
The CRKD necks use a PY32F002B ARM chip running a firmware that simply sends and receives data over a UART connetion to signal the state of the frets and the other inputs. the LED is simply driven directly from the body, and its assumed its a WS2812 style LED chain.

the protocol in use is structured like the following:

```c

struct NeckToBody {
    uint8_t header[2] = {0xA5, 0x01};
    uint8_t len = 0x0C;
    uint8_t padding[2] = {0x00, 0x00};
    uint8_t green:1;
    uint8_t red:1;
    uint8_t yellow:1;
    uint8_t blue:1;
    uint8_t orange:1;
    uint8_t :3;
    uint8_t dpadUpDown; // none: 0x80, up: 0x00, down: 0xFF
    uint8_t dpadLeftRight; // none: 0x80, right: 0x00, left: 0xFF
    uint8_t footer[3] = {0x00, 0x01, 0x15};
    uint8_t crc; // CRC-8/MAXIM-DOW
}

struct BodyToNeck {
    uint8_t data [] = {0xA5, 0x01, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00 };
    uint8_t crc = 0x0E; // CRC-8/MAXIM-DOW
}
```

For calculating the CRC, one can use the following:
```c
unsigned crc8maximdow(unsigned char *data, size_t len) {
    unsigned crc = 0;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (unsigned k = 0; k < 8; k++)
            crc = crc & 1 ? (crc >> 1) ^ 0x8c : crc >> 1;
    }
    return crc;
}
```