---
sort: 4
---
# Wiring a Turntable
If you are new to this, read the [following guide](https://santroller.tangentmc.net/wiring_guides/general.html) for an overview on everything you need to know for modifying a controller.

## Supplies
* A microcontroller from the list in the [following guide](https://santroller.tangentmc.net/wiring_guides/general.html)

* Some Wire
* Soldering Iron
* Multimeter (it will be used mainly in continuity mode, where it beeps when the two contacts are shorted together)
* Wire Strippers
* Wire Cutters
* Heatshrink

## Wiring Steps
1. Open the main section from your turntable
2. If you want the face buttons to work, then you will need to disconnect the original microcontroller from the main pcb. It is on its own seperate daughterboard, so you can just cut all the pins and seperate it. Be extremely careful doing this though, as the traces on the DJ Hero turntable are very low quality, and are extremely easy to break. Once you have done this, follow the traces and find all the different grounds, and connect them to the ground on your microcontroller. Then connect each buttons signal up to seperate digital inputs on your microcontroller.
3. The effects knob is just a potentiometer, so one of the outer pins goes to VCC, the other to GND, and the middle pin goes to an analog pin on your microcontroller.
4. Wire the euphoeria button
    1. Connect V to VCC on your microcontroller.
    2. Connect AT to a digital pin on your microcontroller.
    3. Connect V0 to ground on your microcontroller.
    4. Connect AT to a pin that supports analog outputs on your microcontroller. Use the configuration tool to figure this out, as it will only allow you to select a supported pin.
5. Wire up the Crossfader / Turntable connection board. 
    1. There is a three wire ribbon cable going to this board, and this is for the Crossfader. This is also a potentiometer, so the middle pin needs to go to an analog pin on your microcontroller, and one of the outer pins needs to go to VCC, and the other goes to ground.
    2. The board has connectors that go to little boards that connect to the Turntable platters. These are labeled V C D and G. 
        1. Hook up V to VCC and G to GND, and then hook up the C to SCL and D to SDA.
        * For a Pi Pico, you should choose pins using the tool, as you can choose but only specific pin combinations work.
        * For an Arduino Pro Micro, Leonardo or Micro, the SDA pin is pin 2 and the SCL pin is pin 3.
        * For an Arduino Uno, pin A4 is SDA and A5 is SCL. Note that on newer arduinos, these pins are also available at the top of the board and are labeled SDA and SCL, but note that these are the same pins, so you can use either.
        * For an Arduino Mega, pin 20 is SDA and pin 21 is SCL.

## Wiring Steps (Peripheral, Pi Pico only)
If you would like your frets to contain LEDs, or want your frets to poll at a different rate to the rest of the turntable, you can use the [Peripheral feature](https://santroller.tangentmc.net/wiring_guides/peripheral.html).
1. Program a second Pi Pico as a peripheral
2. Open up the platter on your turntable
3. Desolder the CLK and DATA from your platter, and solder them to the peripheral pins that you have chosen.
4. Desolder the 3V3 line from the platter and solder it to VBUS on your peripheral. Do the same on the main pico, you should end up with VBUS connected to VBUS via the slip ring and platter connector.
5. Connect CLK and DATA to pins that support I2C on your peripheral Pico.
6. Connect 3V3 out to the 3V3 pin on your platter
7. Disconnect the buttons from the platter, and connect them to digital pins your peripheral Pico.
8. Connect ground on the peripheral to ground on the platter. This should also still be connected to ground on the main Pico.
9. You can then hook up RGB Leds with the [following guide](https://santroller.tangentmc.net/wiring_guides/leds.html) if desired. These can be connected to the peripheral pico to allow putting LEDs in the platter.

Note: If using peripheral mode, here are the wire color codes and pinouts:

* YELLOW: DATA (D)
* BLUE: GND (G)
* GREEN: CLK (C)
* RED: 3V3 (V)
* WHITE: SENSE PIN (ID)

Now that you have wired your turntable, go [configure it](https://santroller.tangentmc.net/tool/using.html).
