---
sort: 6
---

# Building an Adaptor for a PS2 Controller

## Supplies

- A microcontroller from the list in the [following guide](https://santroller.tangentmc.net/wiring_guides/general.html)
- For any 3.3v microcontrollers, you will need a 3.3v voltage regulator.
- PS2 Controller socket or some male dupont wires
- 2x 1kΩ Resistor
- Some Wire
- A Soldering Iron

Be careful that you don't ever provide 5v power to any pins on the PS2 Controller. While some people have done this successfully and just use their controllers with 5v, I can't recommend this approach as the controllers are really designed for 3.3v, so it may damage them slowly over time.

## The finished product (When using a 3.3v Pro Micro and dupont wires)

[![Finished adaptor](/assets/images/adaptor-ps2.jpg)](/assets/images/adaptor-ps2.jpg)

## Steps

Refer to the following image for the pinout of a PS2 controller.

[![Adapter pinout](/assets/images/ps2-pinout.jpg)](/assets/images/ps2-pinout.png)

1. Connect Attention, Acknowledge to Acknowledge, Command to MOSI, Data to MISO and Clock to SCK. Note that for anything that isn't the 3.3v Pro Micro or pi pico, you will need to connect these pins using a level shifter.

   | Microcontroller            | SCK | MOSI | MISO | Acknowledge | Attention |
   | -------------------------- | --- | ---- | ---- | ----------- | --------- |
   | Pi Pico                    | GP6 | GP3  | GP4  | GP7         | GP10      |
   | Pro Micro, Leonardo, Micro | 15  | 16   | 14   | 7           | 10        |
   | Uno                        | 13  | 11   | 12   | 2           | 10        |
   | Mega                       | 52  | 51   | 50   | 2           | 10        |

2. Connect a 1kΩ resistor between MISO and 3.3v.
3. Connect a 1kΩ resistor between Acknowledge and 3.3v.
4. Connect the 3.3v pin on the ps2 controller to the 3.3v pin on your microcontroller. For microcontrollers that can output 3.3v natively, you can hook this straight up to 3.3v, but for a 5v only board like the Pro Micro 5v you will need to use a 3.3v voltage regulator to get a stable 3.3v voltage.
5. Connect the gnd pin on the wii breakout / extension cable to the gnd on your microcontroller.

Now that you have wired your adapter, go [configure it](https://santroller.tangentmc.net/tool/using.html).
