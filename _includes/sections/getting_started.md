<details>
    <summary>Electronics basics</summary>    

This guide assumes that you know how to solder. If you do not, stop now and go practice soldering.

If you have no idea what a microcontroller is, start here. If you are familiar with microcontrollers and PCBs skip to [choosing a microcontroller](#choosing-a-microcontroller).

A microcontroller is a small board that can be programmed to perform various functions. In this guide, you will be using the microcontroller to replace the main board of the guitar. Around the edge of the microcontroller, you will see various solder through-holes called pins. These are labelled with a pinout (which you can find by searching for your {your microcontroller} pinout) and sometimes on the microcontroller itself. The pins have various functions, but for this controller mod, you will need to be familiar with all 4 types of pins:
  * GND (ground) - the common connection that all electrical components must connect to in one way or another in order to complete the circuit. Every set of functions for the guitar will feature a GND pin. On the Pi Pico, you will also see a AGND pin. This is also functionally just another GND, however it is recommended to use this for analog inputs like a stick or whammy.
  * V<sub>CC</sub> (common collector voltage) - this is where you would connect to provide power to a circuit. Sometimes this pin is labelled as 5V, 3.3V or 3v3 on the microcontroller or pinout.
  * Digital Pins - These pins are the "basic" pins. They can essentially only show if a button is pressed or not pressed. You will use these for strum, start/select and frets. The are the most common pin on your microcontroller and are usually labelled with just a number. 
  * Analogue Pins - These pins provide a range of values rather than just on and off. These are the pins you will need to use for whammy, tilt (when using an accelerometer), or your joystick. Analogue pins are labeled on the pinout with A followed by a number. 

 ```note
Analogue pins can be used as a digital pin, but digital pins CANNOT be used as an analogue pin. So if it the instructions say connect to a digital pin, you can use an analogue pin.
```
 ```note
V<sub>CC</sub> and GND can have more than one wire soldered to them. For example, if you want tilt and whammy but only have one V<sub>CC</sub> you can solder them both to the single V<sub>CC</sub> pin on the microcontroller.
```

```danger
Avoid pins 0 and 1 on the Arduino Uno, as these get used for sending controller information over USB. 
```

 ```danger
Note that on the pi pico you need to use the `3v3 out` pin (pin 36)  for your V<sub>CC</sub>, not 5V or `3v3_en`. The pins on the pico are not rated for 5v, and the `3v3_en` pin is actually an input that will stop your pico from starting.
```

If you are unfamiliar with microcontrollers, you may also be unfamiliar with PCBs in general and figuring out which pins correlate to which function. Here are some terms you will need to know. 
  * PCB (printed circuit board) - the often green or brown boards containing traces and electronic components
  * Traces - the copper conductors on a board. They look flat and are often metalic. Think of them as "wires" on the board itself connecting components. If you are still confused, a quick google search can help you understand traces.
</details>