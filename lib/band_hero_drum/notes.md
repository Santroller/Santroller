PS3 and Xbox 360 band hero drum kits actually reuse the mainboard from the DJ Hero turntables and Guitar Hero 5 guitars.
As a result, they have multiple processors in the brain, much like how the gh5 guitar had a second processor in the neck.

If you open up the brain you can desolder the mainboard and you are left with the I2C connection and connections for all the buttons.

The brain has an I2C addres of 0x0D, and data is read in from register 10. Unlike the other two instruments, there is no poll rate cap here.

The drum packets are just MIDI, but they start with some nibble that increases and then wraps around, and then give you a count of how many packets are buffered and ready to be read. The MIDI data is 3 bytes long but if you read more than that you just get multiple packets at once.

The data format is:
`0x<counter>0 0x<packetsinbuffer>0 <midi packet>+`