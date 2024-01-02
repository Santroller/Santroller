---
sort: 3
---
# Wiring a Drum Kit
If you are new to this, read the [following guide](https://santroller.tangentmc.net/wiring_guides/general.html) for an overview on everything you need to know for modifying a controller.

## Supplies
* A microcontroller from the list in the [following guide](https://santroller.tangentmc.net/wiring_guides/general.html)

* A 4051 or 4067 based analog multiplexer (Pi Pico only)

* Some Wire
* Soldering Iron
* Multimeter (it will be used mainly in continuity mode, where it beeps when the two contacts are shorted together)
* Wire Strippers
* Wire Cutters
* Heatshrink

## Wiring Steps
1. Open your drum kit.
2. Find the mainboard. Then, find the wires going to the various pads.
3. Use your multimeter to figure out which wires on the various pads are joined together. This will be your common ground, and you can connect one of these to the ground pin on your microcontroller.
4. The other wires for each pad are the signal pins, and these need to go to seperate analog pins on your microcontroller.
5. If you would like to also hook up face buttons, you will need to follow the traces on the main PCB and work out the signal pins for each button, and hook those up to digital pins on your microcontroller.

Now that you have wired your drum, go [configure it](https://santroller.tangentmc.net/tool/using.html).

## Drum Specific Configuration
Set the device type to one of the various drum modes, depending on the type of kit you are using. Each pad and cymbal will then have its own axis. Bind the pins to the correct axis, and then you should see the axis' react when you strike the drum pads.

### Calibration
Drum calibration is a bit different to most other controllers, as an automated calibration method does not really work here. A drum axis has a `minimum` and a `maximum`. The minimum is the smallest value that will be considered a hit by the drum kit. Increase this value if your drum is registering hits that it shouldn't. The maximum specifies the hardest hit you want the drum to register. This gets used for games that rely on hitting the drum hard to do different actions, and the actual resulting analog value will be scaled between the min and max.
If your drum is triggering when you aren't hitting the pad, then you need to increase the minimum, if it isn't reacting on light hits then the minimum needs to be decreased.

### Debounce
You will also need to adjust the debounce. The debounce for a drum hit dictates how long the drum signal will stay active, so you want to increase this a lot for a drum kit, otherwise you will find that your drum hits will immediately turn off which is not what the games expect.
