# DJH Turntable Platter

The DJH turntable platter communicates with the main base via I2C. Each platter has a different I2C address, 0x0E for the left and 0x0D for the right. A read from address 0x12 will give you the state of the platter.
Note that the platters velocity is a delta from the previous poll, so this gives you a limit on how fast the platter can be polled. If you poll it too slow the values will be very large, and if you poll too quickly, the values will be tiny and you will end up losing a lot of the range, the wii remote polls at 5ms, so this is usually want you want to poll at.

Format of the data is the following:

```c
struct djh_data {
    uint8_t :3;
    uint8_t green;
    uint8_t red;
    uint8_t blue;
    uint8_t :2;
    uint8_t :8;
    int8_t velocity;
}
```