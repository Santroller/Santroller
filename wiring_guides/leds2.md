---
sort: 9
---
# Adding Standard LEDs inline on a controller
If you don't want to use APA102s, and just want to hook up normal LEDs from buttons, you can follow the following guide.
## Supplies
* A controller modded using any of the guides
* Some LEDs
* The correct current limiting resistor for your specific LEDs

## The finished product
[![Finished adaptor](/assets/images/inline-led.jpg)](/assets/images/inline-led.jpg)

## Schematic
[![Schematic](/assets/images/led.png)](/assets/images/led.png)

Note that you can use whatever pins you want as your digital pins, this is just for example
And node that the 220ohm resistor value is also for demonstration, you will need to calculate the correct resistor for your specific LEDs.
Usually LEDs will have a recommended resistor in their datasheets, but there are also calculators online.

## Steps
1. Hook all the anodes of the LEDs to the 5V pin on your microcontroller (note that for the pi pico, this is VBUS).
2. Hook up the cathodes of the LEDs to one side of each fret
3. Hook the other side of the fret up to a digital pin on your microcontroller
4. Hook up a 500 ohm resistor in parallel between each digital pin and ground. You should have two connections to each digital pin, one going to the fret, and one going to a resistor that then goes to ground.

Now that you have wired your LEDs, go [configure them](https://santroller.tangentmc.net/tool/using.html).