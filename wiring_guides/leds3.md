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
2. Wire Vdd on your drivers to VCC on your microcontroller
3. Connect the SCK (CLK) pin on each driver to your microcontroller.
   If you are using a pi pico, you can choose pins using the configuration tool, as only specific groups of pins work.

   | Microcontroller            | SCK (CI) |
   | -------------------------- | -------- |
   | Pro Micro, Leonardo, Micro | 15       |
   | Uno, Pro Mini              | 13       |
   | Mega                       | 52       |

4. Connect the MOSI (SDI) on the first driver to the MOSI pin on your microcontroller. Then chain the MOSI (SDO) outputs to the MOSI (SDI) on the next driver, until all drivers are connected.
   If you are using a pi pico, you can choose pins using the configuration tool, as only specific groups of pins work.

   | Microcontroller            | MOSI (DI) |
   | -------------------------- | --------- |
   | Pro Micro, Leonardo, Micro | 16        |
   | Uno, Pro Mini              | 11        |
   | Mega                       | 51        |

5. Connect your LEDs to the OUTn pins. The cathode of each LED connects to an OUTn pin, while the anodes are all connected together and connect to the voltage supply for your LEDs. Depending on how many LEDs you are using, it may make sense to use an external supply here, which would need to have its VCC connected to the anodes of the LEDs, and its GND connected to GND on the drivers.
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
