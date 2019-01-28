# Wii Xbox360 Adapter
![https://i.imgur.com/QeoAx7B.jpg](https://i.imgur.com/QeoAx7B.jpg)

This project aims to provide an adapter that when used will expose a wii guitar hero controller as an xbox 360 controller, so that games such as Clone Hero can immediately use these controllers without calibration. Note that this does not take care of authentication, so your controllers will not work on a real xbox 360.

# How to use this repo
## You will need:
1. An Arduino with USB hid compatibility and 3.3v compatibility (pro micro recommended)
2. A wii extension cable or breakout board
3. A MPU6050 or similar

## Flashing the software
1. Install an AVR toolchain + make
2. run make avrdude
3. For updates, hit + and - to jump to bootloader mode.

## Building the hardware
1. Find the I2C pins on your Arduino, and connect them to the extension / breakout board. Also connect ground to ground and 3.3v to 3.3v on the Arduino.
2. Connect your MPU6050 to the same pins. Note that the interrupt pin is not used in this sketch.
3. Use double sided tape to adhere the MPU6050 to the guitar. Note that the MPU6050 needs to be placed in a horizontal position during normal use.

## Used projects:
https://github.com/bootsector/XInputPadMicro
https://github.com/dmadison/NintendoExtensionCtrl
https://github.com/abcminiuser/lufa/
