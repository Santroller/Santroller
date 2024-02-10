---
sort: 6
---

# Building an Adaptor for a PS2 Controller

## Supplies

- A microcontroller from the list in the [following guide](https://santroller.tangentmc.net/wiring_guides/general.html)
- For any 5v microcontrollers, you will need a 3.3v voltage regulator.
- PS2 Controller socket
- 2x 1kΩ Resistor
- Some Wire
- A Soldering Iron

Be careful that you don't ever provide 5v power to any pins on the PS2 Controller. While some people have done this successfully and just use their controllers with 5v, I can't recommend this approach as the controllers are really designed for 3.3v, so it may damage them slowly over time.

## Steps

Refer to the following image for the pinout of a PS2 controller.

[![Adapter pinout](/assets/images/ps2-pinout.png)](/assets/images/ps2-pinout.png)

1. Connect pins according to the below table. Note that for anything that isn't the 3.3v Pro Micro or pi pico, you will need to connect these pins using a level shifter.

   For beginners we provide a recommended pinout below for each microcontroller. These pinouts are the same pinouts that are used on the old Ardwiino firmware, so if you have an old device, this is the pinout it will be using.
   If these pins don't work for you, you can use one of the advanced pinouts below. Note that the Pi Pico has multiple channels, and SCK, MOSI and MISO need to be from the same channel.

   [![pinout](/assets/images/ps2.png)](/assets/images/ps2.png)

   | Microcontroller                          | SCK / Clock / Blue | MOSI / Command / Orange | MISO / Data / Brown | Acknowledge / Green    | Attention / Yellow |
   | ---------------------------------------- | ------------------ | ----------------------- | ------------------- | ---------------------- | ------------------ |
   | Pi Pico (Recommended)                    | GP6                | GP3                     | GP4                 | GP7                    | GP10               |
   | Pro Micro, Leonardo, Micro (Recommended) | 15                 | 16                      | 14                  | 7                      | 10                 |
   | Uno (Recommended)                        | 13                 | 11                      | 12                  | 2                      | 10                 |
   | Mega (Recommended)                       | 52                 | 51                      | 50                  | 2                      | 10                 |
   | Pi Pico (Advanced, Channel 0)            | GP2, GP6, GP18     | GP3, GP7, GP19          | GP0, GP4, GP16      | Any                    | Any                |
   | Pi Pico (Advanced, Channel 1)            | GP10, GP14         | GP11, GP15              | GP8, GP12           | Any                    | Any                |
   | Pro Micro, Leonardo, Micro (Advanced)    | 15                 | 16                      | 14                  | 0, 1, 2, 3 or 7        | Any                |
   | Uno (Advanced)                           | 13                 | 11                      | 12                  | 2 or 3                 | Any                |
   | Mega (Advanced)                          | 52                 | 51                      | 50                  | 2, 3, 18, 19, 20 or 21 | Any                |

2. Connect a 1kΩ resistor between MISO and 3.3v.
3. Connect a 1kΩ resistor between Acknowledge and 3.3v.
4. Connect the 3.3v pin on the ps2 controller to the 3.3v pin on your microcontroller. For microcontrollers that can output 3.3v natively, you can hook this straight up to 3.3v, but for a 5v only board like the Pro Micro 5v you will need to use a 3.3v voltage regulator to get a stable 3.3v voltage.
5. Connect the gnd pin on the PS2 controller to the gnd on your microcontroller.

Now that you have wired your adapter, go [configure it](https://santroller.tangentmc.net/tool/using.html).
