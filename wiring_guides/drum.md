---
sort: 3
---
# Wiring a Drum Kit
If you are new to this, read the [following guide](https://santroller.tangentmc.net/wiring_guides/general.html) for an overview on everything you need to know for modifying a controller.

## Supplies
* A microcontroller from the list in the [following guide](https://santroller.tangentmc.net/wiring_guides/general.html)

* A 4051 or 4067 based analog multiplexer (Only necesssary for the Pi Pico, as other microcontrollers will have enough analog pins for a drum kit)

* Some Wire
* Soldering Iron
* Multimeter (it will be used mainly in continuity mode, where it beeps when the two contacts are shorted together)
* Wire Strippers
* Wire Cutters
* Heatshrink

### Supplies for a custom kit
* 1MOhm resistor per drum pad
* Piezo sensor

## Wiring Steps (Reusing the original PCB from an existing kit)
1. Open your drum kit.
2. Find the mainboard. Then, find the wires going to the various pads.
3. Grab a multimeter in contunity mode, and test contunity between the wires going to the different pads. You should find that there is a common connection to each pad, which is your ground connection. Connect this to the ground on your microcontroller.
4. The other wires for each pad are the signal pins, and these need to go to seperate analog pins on your microcontroller or multiplexer.
5. If you would like to also hook up face buttons, you will need to follow the traces on the main PCB and work out the signal pins for each button, and hook those up to digital pins on your microcontroller.
6. Cut any traces going to the original microcontroller on the old drum brain.

## Wiring Steps (Custom kit or wiring directly and bypassing the original PCB)
1. Wire the red wire from each piezo to an analog pin on your microcontroller or a multiplexer.
2. Wire the black wires to ground on the microcontroller or multiplexer.
3. Place a 1MOhm resistor between the black and red wires on each piezo.

## Wiring the multiplexer (only really necessary for the Pi Pico, as most other microcontrollers have enough analog pins)
1. Wire Vdd on the multiplexer to 3v3 on the Pico.
2. Wire Vss to GND on your Pico 
3. If your multiplexer has a Vee, also wire that to ground on the Pi Pico. Vee allows for using the multiplexer with negative voltages, but since we aren't doing this we set it to ground to disable that feature.
4. Also wire INH / Inhibit to ground, if this exists on your multiplexer. This pin disables the I/O if it is driven high, so we ground it to make sure the chip is always enabled.
5. Wire the analog output (Often labelled COM or common in/out, but also labelled SIG on some breakout boards) on the multiplexer to an analog pin on your Pi Pico.
6. Wire A/S0, B/S1 and C/S2 (and D/S3 for the 16 channel multiplexer) to seperate digital pins on your Pi Pico.
7. Wire each drum pad to a different channel on the multiplexer.
8. When configuring, specify all these pins, and then specify the correct channel for a given pad.

Note: close the tool if you want to use your drum kit, the multiplexer is overridden by the tool when it updates its sensor values, and this will override the controller inputs when it happens.

Now that you have wired your drum, go [configure it](https://santroller.tangentmc.net/tool/using.html).

## Drum Specific Configuration
Set the device type to one of the various drum modes, depending on the type of kit you are using. Each pad and cymbal will then have its own axis. Bind the pins to the correct axis, and then you should see the axis' react when you strike the drum pads.

### Calibration
Drum calibration is a bit different to most other controllers, as an automated calibration method does not really work here. A drum axis has a `minimum` and a `maximum`. The minimum is the smallest value that will be considered a hit by the drum kit. Increase this value if your drum is registering hits that it shouldn't. The maximum specifies the hardest hit you want the drum to register. This gets used for games that rely on hitting the drum hard to do different actions, and the actual resulting analog value will be scaled between the min and max.
If your drum is triggering when you aren't hitting the pad, then you need to increase the minimum, if it isn't reacting on light hits then the minimum needs to be decreased.
If your drum reacts when you hit other pads, this is also a indication that you need to increase the minimum.

### Debounce
You will also need to adjust the debounce. The debounce for a drum hit dictates how long the drum signal will stay active, so you want to increase this a lot for a drum kit, otherwise you will find that your drum hits will immediately turn off which is not what the games expect.