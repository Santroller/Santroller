---
sort: 9
---

# Adding Standard LEDs with a STP16CPC26 driver

If you just wish to use a lot of standard LEDs, you can also opt to use 1 or many STP16CPC26 LED drivers. You will need one driver for every 16 leds you want to connect.

## Supplies

- A controller modded using any of the guides
- Some LEDs
- Enough STP16CPC26 drivers to drive your leds

## Steps

1. Wire GND on your drivers to GND on your microcontroller
2. Wire V<sub>DD</sub> on your drivers to V<sub>CC</sub> on your microcontroller
3. Connect the SCK (CLK) pin on each driver to your microcontroller.
   If you are using a Pi Pico and these pins don't work for you, you can use one of the advanced pinouts below. Note that the Pi Pico has multiple channels, and SCK and MOSI need to be from the same channel.

   | Microcontroller               | SCK (CLK)        |
   | ----------------------------- | ---------------- |
   | Pi Pico (Recommended)         | GP6              |
   | Pro Micro, Leonardo, Micro    | 15               |
   | Uno                           | 13               |
   | Mega                          | 52               |
   | Pi Pico (Advanced, Channel 0) | GP2, GP6, GP18   |
   | Pi Pico (Advanced, Channel 1) | GP10, GP14, GP26 |

4. Connect the MOSI (SDI) on the first driver to the MOSI pin on your microcontroller. Then chain the MOSI (SDO) outputs to the MOSI (SDI) on the next driver, until all drivers are connected.
   If you are using a Pi Pico and these pins don't work for you, you can use one of the advanced pinouts below. Note that the Pi Pico has multiple channels, and SCK and MOSI need to be from the same channel.

   | Microcontroller               | MOSI (SDI)       |
   | ----------------------------- | ---------------- |
   | Pi Pico (Recommended)         | GP3              |
   | Pro Micro, Leonardo, Micro    | 16               |
   | Uno                           | 11               |
   | Mega                          | 51               |
   | Pi Pico (Advanced, Channel 0) | GP3, GP7, GP19   |
   | Pi Pico (Advanced, Channel 1) | GP11, GP15, GP27 |

5. Connect your LEDs to the OUTn pins. The cathode of each LED connects to an OUTn pin, while the anodes are all connected together and connect to the voltage supply for your LEDs. Depending on how many LEDs you are using, it may make sense to use an external supply here, which would need to have its V<sub>CC</sub> connected to the anodes of the LEDs, and its GND connected to GND on the drivers.
6. Connect a resistor between R-EXT and GND, depending on how much current you want to drive each LED with.

   | Resistor (ohms) | Output Current (mA) |
   | --------------- | ------------------- |
   | 4300            | 5                   |
   | 2000            | 10                  |
   | 1000            | 20                  |
   | 470             | 40                  |
   | 330             | 60                  |
   | 220             | 90                  |

Now that you have wired your LEDs, go [configure them](https://santroller.tangentmc.net/tool/using.html).
