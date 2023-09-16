# Wiring a Drum Kit
## Before you start:
If you are new to this, read the [following guide](https://santroller.tangentmc.net/wiring_guides/general.html) for an overview on everything you need to know for modifying a controller.

## Supplies
* A microcontroller from the list in the [following guide](https://santroller.tangentmc.net/wiring_guides/general.html)

* A CD4051 or CD4057 based analog multiplexer (Pi Pico only)

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