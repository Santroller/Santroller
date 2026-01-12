Pro guitar neck is SPI, cs active low, cpol 0, cpha 1, lsb first, 100000hz clock

body -> neck, {0x80, 0x12, 0x12, 0x12, 0x12, 0x12}

neck -> body

struct {
    uint8_t header = 0x80;
    uint16_t lowEFret : 5;
    uint16_t aFret : 5;
    uint16_t dFret : 5;
    uint16_t : 1; // always 1

    uint16_t gFret : 5;
    uint16_t bFret : 5;
    uint16_t highEFret : 5;
    uint16_t : 1; // always 0

    uint8_t : 1; // always 0
    uint8_t green :1;
    uint8_t red :1;
    uint8_t yellow :1;
    uint8_t blue :1;
    uint8_t orange :1;
    uint8_t soloFlag : 1;
    uint8_t : 1; // always 0

} protarneck_t;