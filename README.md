# Wiitar Xbox360 Adapter
![adaptor.jpg](adaptor.jpg)

This project aims to provide an adapter that when used will expose a wii guitar hero controller as an xbox 360 controller, so that games such as Clone Hero can immediately use these controllers without calibration.

# How to use this repo
## You will need:
An Arduino with USB hid compatibility and 3.3v compatibility (pro micro recommended)
A wii extension cable or breakout board
A gravity switch
A 2 pin connector (optional)

## Setting up an Arduino IDE
It is recommended to set aside a separate Arduino IDE for this project, as LUFA will overwrite many core Arduino USB files, and as a result will lead to some issues with other sketches.

1. Follow the guide at https://github.com/Palatis/Arduino-Lufa to set up a LUFA environment.

2. Install https://github.com/dmadison/NintendoExtensionCtrl
3. If using a 3.3v micro, copy boards.txt to <Arduino install>/hardware/Arduino/avr
4. Upload the sketch to the Arduino. Note that for a pro micro, you will need to reset the Arduino twice in order for it to be recognised as an Arduino.

## Building the hardware
1. Find the I2C pins on your Arduino, and connect them to the extension / breakout board. Also connect ground to ground and 3.3v to 3.3v on the Arduino.
2. Connect the gravity switch between digital pin 4 and ground. Optionally put some sort of connector between the gravity switch and the Arduino if you would like this to be easily removable.
3. Use double sided tape to adhere the gravity switch to the guitar. Note that the angle the switch is mounted can be used to calibrate when the switch activates, and so it is useful to open some tool that allows you to visualise the right joystick Y axis in order to calibrate this in a way that suits you.