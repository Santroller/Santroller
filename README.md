# Ardwiino
![https://i.imgur.com/QeoAx7B.jpg](https://i.imgur.com/QeoAx7B.jpg)

This project aims to provide a AVR solution that supports both direct and WII based guitar configurations.
This project supports acting as either a keyboard, gamepad or xinput device.

Note that xinput mode does not take care of xinput security, so your controllers will not work on a real xbox 360.

This repository contains code for both the uno, and the pro micro. It could very easily be modified to support many other AVR microcontrollers, as long as the AVR supports HID, either directly or via another AVR microcontroller (aka the uno).
It is recommended to use https://github.com/sanjay900/guitar-configurator for configuring, as it takes care of everything.

# How to use this repo (for a wii adaptor)
## You will need:
1. An AVR microcontroller with USB hid compatibility and 3.3v compatibility (pro micro recommended)
2. A wii extension cable or breakout board
3. A MPU6050 or similar (optional)

## Building the hardware
1. Find the I2C pins on your Arduino, and connect them to the extension / breakout board. Also connect ground to ground and 3.3v to 3.3v on the Arduino.
2. Connect your MPU6050 to the same pins. Note that the interrupt pin is not used in this sketch.
3. Use double sided tape to adhere the MPU6050 to the guitar. Note that the MPU6050 needs to be placed in a horizontal position during normal use.

![pcb/ardwiino_schematic.png](pcb/ardwiino_schematic.png)


## Used projects:
* https://github.com/bootsector/XInputPadMicro
* https://github.com/abcminiuser/lufa/
