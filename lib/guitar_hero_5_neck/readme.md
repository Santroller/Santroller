# GH5 neck

The GH5 neck communicates with the body via I2C. It has an I2C address of 0x0D. A read from address 0x12 will give you the state of the buttons and the slider.
The neck can be polled pretty fast but it has a cap at how fast it actually polls its inputs internally. As a result, There ends up being an average poll rate of around 4ms when using one of these.

The chip we are talking to is labeled as a HA2002-I/SS, and it is a rebadged PIC16LF722. The firmware for it is stored next to this file as gh5-neck-fw.hex.

Format of the data is the following:

```c
struct gh5_data {
    uint8_t orange;
    uint8_t :2;
    uint8_t green;
    uint8_t red;
    uint8_t yellow;
    uint8_t blue;
    uint8_t :8;
    uint8_t slider;
}
```

For information on the slider, visit https://github.com/TheNathannator/PlasticBand/blob/main/Docs/Instruments/5-Fret%20Guitar/Guitar%20Hero/General%20Notes.md