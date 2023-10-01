---
sort: 1
---
# Getting started

This guide assumes that you know how to solder, if you do not stop now and go practice soldering.

If you have no idea what a microcontroller is, start here. If you are familiar with microcontrollers and pcbs skip to [choosing a microcontroller](#choosing-a-microcontroller).

A microcontroller is a small board that can be programmed to perform various functions. In this guide you will be using the microcontroller to replace the main board of the guitar. Around the edge of the microcontroller, you will see various solder through holes called pins. These are labelled with a pinout (which you can find by googling {your microcontroller} pinout) and sometimes on the microcontroller itself. The pins have various functions, but for this controller mod you will need to be familiar with 4 kinds:
  * GND (ground) - the common connection that all electrical components must connect to in one way or another in order to complete the circuit. Every set of functions for the guitar will feature a GND pin.
  * VCC (voltage common connector) - this is where you would connect to provide power to a circuit. Sometimes this pin is labelled as 3.3V or 3v3 on the microcontroller or pinout.
  * Digital Pins - These pins are the "basic" pins. They can essentially only show if a button is pressed or not pressed. You will use these for strum, start/select and frets. The are the most common pin on your microcontroller and are usually labelled with just a number. 
  * Analogue Pins - These pins provide a range of values rather than just on and off. These are the pins you will need to use for whammy, tilt (when using an accelerometer), or your joystick. Analogue pins are labeled on the pinout with A followed by a number. 

 ```note
Analogue pins can be used as a digital pin, but digital pins CANNOT be used as an analogue pin. So if it the instructions say connect to a digital pin, you can use an analogue pin.
```
 ```note
VCC and GND can have more than one wire soldered to them. For example, if you want tilt and whammy but only have one VCC you can solder them both to the single VCC pin on the microcontroller.
```

```danger
Avoid pins 0 and 1 on the Arduino Uno, as these get used for sending controller information over USB. 
```

 ```danger
Note that on the pi pico you need to use the `3v3 out` pin (pin 36)  for your VCC, not 5V or `3v3_en`. The pins on the pico are not rated for 5v, and the `3v3_en` pin is actually an input that will stop your pico from starting.
```

If you are unfamiliar with microcontrollers, you may also be unfamiliar with PCBs in general and figuring out which pins correlate to which function. Here are some terms you will need to know. 
  * PCB (printed circuit board) - the often green or brown boards containing traces and electronic components
  * traces - the copper conductors on a board. They look flat and are often metalic. Think of them as "wires" on the board itself connecting components. If you are still confused, a quick google can help you understand traces.

# Choosing a microcontroller
I recommend the Pi Pico, as it supports every feature and just tends to make everything much less complicated, as it runs at 3.3v and this tends to simplify a lot of adapters and support with many peripherals.

However, below is a comparison of the various micro controllers that are supported by the configuration tool.

## Pi Pico 
This micro controller was created by the Raspberry Pi foundation, so it means that buying one supports them instead of random companies cloning Arduinos. They are considerably more powerful than any of the other supported Arduinos. The Pi Pico supports a few extra features, such as USB Input (which allows for XB1 / Series compatability), Peripheral microcontrollers and the GHWT tap bar. Note that the Pi Pico runs at 3.3v, so it can just be used as is for PS2 and Wii adapters. Pi Picos also tend to be quite cheap and often have the advantage that they can often be brought at local stores. With the currrent firmware, picos work much much better than arduinos if your goal is to create adaptors, but they also work better for direct wiring too. There are many boards based on the Pi Pico as well, and all of these work just fine with the firmware.

## Sparkfun Pro Micro (5v)
The 5V pro micro will work okay for direct wiring, but being that it runs at 5V, it will require voltage conversion to build Wii adapters and PS2 adapters and turntables.

## Sparkfun Pro Micro (3.3v)
If you want to build an adapter for a Wii or PS2 guitar, then this will be easier to use than any of the 5v microcontrollers. Due to the lower voltage, these do run at half the speed of the 5v variants, a. Clones of the Pro Micro are quite cheap but will need to be purchased from ebay or aliexpress, real Pro Micros are quite expensive but there isn't really much of a difference. 3.3V arduinos will poll a wii guitar slightly slower than a 5V arduino but in practise this does not end up mattering as there are a lot of other delays necessary for communicating with a wii guitar.

## Arduino Micro
This is essentially the same thing as a 5v pro micro, however these are officially made by Arduino. These are often a bit more expensive as they aren't really cloned.

## Arduino Leonardo
This is essentially a 5v pro micro with the layout of a Arduino Uno, so you get more pins but it is also much larger. You can get clones of these, but they are still more expensive than pro micros or picos.

## Arduino Uno (r1/r2/r3)
These micro controllers are actually two micro controllers in one, and they work in tandem to provide a working controller. This has its disadvantages, as code needs to keep these controllers in sync, and this can result in issues if a bad configuration is programmed, and generally results in requiring more complicated and optimised code to work. Unos do still get 1000hz, but I would recommend against them if you are buying a new Arduino. Note that some clone Arduino Unos are actually missing the second micro controller, and these ones will NOT work at all. If you see an Arduino Uno listing that mentions "ch340g" or something along those lines do not purchase it. Due to this, they are harder to purchase and since they require more parts, they are more expensive than a Pro Micro or Pi Pico.

## Arduino Mega 2560
These are in the same situation as the Uno, however the main micro controller has a lot more pins. These do also end up being rather expensive due to the sheer amount of parts that are needed to make one.

## Arduino Uno r4
This is the newest entrant to the Arduino Uno line, and it is a totally different microcontroller and hence it is not supported.

## Arduino Mini or Nano or Pro Mini or Pro Nano
These do NOT work, as they are essentially Unos that lack the second micro controller that allow for custom USB device emulation.
