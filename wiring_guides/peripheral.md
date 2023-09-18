---
sort: 7
---
# Peripheral controllers (Pi Pico Only)
This firmware has a concept of a peripheral controller. This allows you to connect four wires between two Pi Picos, which can allow for connecting inputs or RGB Leds over connectors with limited pins. This is useful for Guitars and Turntables, as it allows for wiring frets in a responsive way over a limited number of pins. Note that the peripheral feature uses I2C, so it can actually share pins with a DJ Hero Turntable Platter, or a GH5 Guitar neck.

```danger
LEDs require 5 volts, but most I2C devices run on 3.3 volts. This means if you intend to share pins with a DJ Hero Turntable Platter, or a GH5 guitar neck, you will need to redirect power for that device to the peripheral's 3.3v output, as we need a 5 volt signal sent to the peripheral. 
```

## You will need
* A device modded using one of the other guides, with a Pi Pico
* Another Pi Pico

## Steps
1. Connect VBUS to VBUS and GND to GND
2. Note that if you are sharing pins between the peripheral and another device, you will need to disconnect the VCC from the original device and connect its VCC to 3v3 on your pico. This is because VBUS is a 5V rail, and devices like the GH5 neck and the DJ Hero turntable platter run on 3.3v.
2. Using the configuration tool, choose a valid SDA and SCL pair, and connect SDA to SDA and SCL to SCL.
3. Connect your inputs to the peripheral, following the other guides.
4. Connect your LEDs to the peripheral if desired, following the [RGB LED guide](https://santroller.tangentmc.net/wiring_guides/leds.html)

Now that you have wired your peripheral, go [configure it](https://santroller.tangentmc.net/tool/using.html). Note that the peripheral does not need to be configured after it is programmed, it receives commands from the main micro controller instead of having its own configuration.