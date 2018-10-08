# Wii Xbox360 Adapter
![https://i.imgur.com/QeoAx7B.jpg](https://i.imgur.com/QeoAx7B.jpg)

This project aims to provide an adapter that when used will expose a wii guitar hero controller as an xbox 360 controller, so that games such as Clone Hero can immediately use these controllers without calibration. Note that this does not take care of authentication, so your controllers will not work on a real xbox 360.

# How to use this repo
## You will need:
1. An Arduino with USB hid compatibility and 3.3v compatibility (pro micro recommended)
2. A wii extension cable or breakout board
3. A ADXL335 or similar accelerometer

## Setting up an Arduino IDE
It is recommended to set aside a separate Arduino IDE for this project, as LUFA will overwrite many core Arduino USB files, and as a result will lead to some issues with other sketches.

1. Follow the guide at https://github.com/Palatis/Arduino-Lufa to set up a LUFA environment.

2. Install https://github.com/dmadison/NintendoExtensionCtrl
3. If using a 3.3v micro, copy boards.txt to <Arduino install>/hardware/Arduino/avr
4. Upload the sketch to the Arduino. Note that for a pro micro, you will need to reset the Arduino twice in order for it to be recognised as an Arduino.

## Building the hardware
1. Find the I2C pins on your Arduino, and connect them to the extension / breakout board. Also connect ground to ground and 3.3v to 3.3v on the Arduino.
2. Connect accelerometers between the analogue pins 0,1,2,3
3. Use double sided tape to adhere the arduino to the guitar.

## Used projects:
https://github.com/bootsector/XInputPadMicro/
https://github.com/Palatis/Arduino-Lufa
https://github.com/dmadison/NintendoExtensionCtrl
