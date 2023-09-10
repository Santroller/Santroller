# Building a RF (Wireless) Guitar and Receiver
## You will need
* Two NRF24l01s
* The parts to build either a wii adaptor or directly wired guitar from their respective guides.
  * For the Arduino, an Arduino Pro Mini is recommended for the arduino that is used as a transmitter, as these use very small amounts of power compared to other arduinos
  * If using an Arduino Pro Mini, you will also need an FTDI breakout for it
  * You will also need batteries for this arduino
* Another arduino to use as a receiver, this can be one of the following:
  * Arduino Pro Micro (3.3v)
  * Arduino Pro Micro (5v)
  * Arduino Leonardo
  * Arduino Micro
  * Arduino Uno
  * Arduino Mega
* Some Wire
* A Soldering Iron
* A multimeter
* Wire Strippers
* Wire Cutters
* Heat shrink

```note
RF transmitting Arduinos (the side that connects to the controller) are programmed to go to sleep after 10 minutes, and then will need to be woken up by their respective wakeup pins, which will be explained later in the guide.
```
```danger
note that on the pi pico you need to use the `3v3 out` pin (pin 36)  for your VCC, not 5V or `3v3_en`. The pins on the pico are not rated for 5v, and the `3v3_en` pin is actually an input that will stop your pico from starting.
```
```danger
PS2 Guitars and RGB leds are incompatible with RF, as they all use the same pins.
```

## The finished product
[![Finished adaptor](../assets/images/rf.jpg){: width="250" }](../assets/images/rf.jpg)

## Steps
1. Connect the VCC and GND pins on each RF module to its respective Arduino.
2. Connect the RF module pins to the following Arduino pin numbers:
3. 
   | Microcontroller | SCK | MOSI | MISO | CE | IRQ | CSN |
   | --- | --- | --- | --- | --- | --- | --- |
   | Pro Micro, Leonardo, Micro | 15 | 16 | 14 | 0 | 1 | 10 |
   | Uno, Pro Mini | 13 | 11 | 12 | 8 | 2 | 10 |
   | Mega | 52 | 51 | 50 | 53 | 2 | 10 |
   
4. Hook a button up to the following pin on your transmitting arduino, this is used for waking up the device from sleep, and can be an existing button. For example, you could wire your start button here and use it to wake the arduino up from sleep.

   | Microcontroller | Wakeup Pin |
   | --- | --- |
   | Pro Micro, Leonardo, Micro | 7 |
   | Uno, Mega, Pro Mini | 3 |

5. Follow the guide of your choice to hook up the controller inputs to the transmitter arduino
   * ![Direct](direct.html)
   * ![Wii](wii.html)
6. Plug in the receiver arduino, open the config tool and program it.
7. Hit the Configure RF button, and then enable RF
8. Click on Program RF Transmitter
9. Plug in your transmitting arduino
10. Program the transmitting arduino
11. Configure the guitar, this is detailed better in the guide specific to the type of guitar your creating
   * ![Direct](direct.html)
   * ![Wii](wii.html)
