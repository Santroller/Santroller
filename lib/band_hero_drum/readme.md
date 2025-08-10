# Support for the MIDI mainboard within PS3 and Xbox 360 Band Hero / GH5 drum kits

These kits actually share a mainboard with DJH turntables, WoR guitars and GH5 guitars. Logic specific to each device (like the drums or the neck) were relegated to seperate microcontrollers.
Santrollers support simply works by us desoldering this shared mainboard and then hooking up to the I2C pins it was using.
This interconnect simply just streams MIDI and thus we treat these devices as MIDI devices.

The brain has an I2C addres of 0x0D, and data is read in from register 10. The microcontroller is much newer in this, so there doesn't appear to be a poll rate cap.

The chip in question is labeled HA2005-I/SS, and this ends up being a rebadged PIC18F25K20. The firmware from this chip is stored here as bh.hex.

The I2C response is structured like the following:
```c
struct packet {
    uint8_t counter : 4;
    uint8_t : 4;
    uint8_t remainingPacketCount : 4;
    uint8_t : 4;
    uint8_t midiPacket[3];
}
```
