---
sort: 7
---

# Peripheral controllers (Pi Pico Only)

This firmware has a concept of a peripheral controller. This allows for using a second Pi Pico as a secondary controller which can then be connected to LEDs or inputs. This is useful for Guitars and Turntables, as it allows for wiring frets in a responsive way over a limited number of pins. Note that the peripheral feature uses I2C, so it can actually share pins with a DJ Hero Turntable Platter, or a GH5 Guitar neck.

```danger
LEDs require 5 volts, but most I2C devices run on 3.3 volts. This means if you intend to share pins with a DJ Hero Turntable Platter, or a GH5 guitar neck, you will need to redirect power for that device to the peripheral's 3.3v output, as we need a 5 volt signal sent to the peripheral.
```

## Supplies

- A device modded using one of the other guides, with a Pi Pico
- Another Pi Pico

## Steps

1. Connect VBUS to VBUS and GND to GND
2. Note that if you are sharing pins between the peripheral and another device, you will need to disconnect the V<sub>CC</sub> from the original device and connect its V<sub>CC</sub> to 3v3 on your pico. This is because VBUS is a 5V rail, and devices like the GH5 neck and the DJ Hero turntable platter run on 3.3v.
3. Connect SCK to SCK and SDA to SDA between the picos. If the recommended pins don't work for you, you can pick your own pins. The pins do not need to be the same on the peripheral and the main pico, however, the SDA and SCL on the peripheral pico need to be in the same channel, and the SDA and SCL on the main pico need to be in the same channel.

   | Microcontroller               | SDA                              | SCL                              |
   | ----------------------------- | -------------------------------- | -------------------------------- |
   | Pi Pico (Recommended)         | GP18                             | GP19                             |
   | Pi Pico (Advanced, Channel 0) | GP0, GP4, GP8, GP12, GP16, GP20  | GP1, GP5, GP9, GP13, GP17, GP21  |
   | Pi Pico (Advanced, Channel 1) | GP2, GP6, GP10, GP14, GP18, GP26 | GP3, GP7, GP11, GP15, GP19, GP27 |

4. Connect your inputs to the peripheral, following the other guides.
5. Connect your LEDs to the peripheral if desired, following the [RGB LED guide](https://santroller.tangentmc.net/wiring_guides/leds.html)

Now that you have wired your peripheral, go [configure it](https://santroller.tangentmc.net/tool/using.html). Note that the peripheral does not need to be configured after it is programmed, it receives commands from the main micro controller instead of having its own configuration.
