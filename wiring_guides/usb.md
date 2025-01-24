---
sort: 7
---
# Setting up an Adaptor for a USB controller (Pi Pico Only)


```danger
If you wish to also use PS2 Emulation, follow the [PS2 controller + USB Host guide](https://santroller.tangentmc.net/wiring_guides/ps2_output_usb_host.html)
```
&nbsp;

```danger
If you wish to also use Wii Extension emulation, follow the [Wii extension emulation + USB Host guide](https://santroller.tangentmc.net/wiring_guides/wii_output_usb_host.html)
```

## Supplies
* A Pi Pico
  * Note that if you don't want to solder, there are boards out there with USB Host set up already, such as the Adafruit Feather RP2040 with USB host.
* A USB Extension cable, or a USB breakout

## Wiring Steps

1. If you are using a USB extension cable, cut it in half and expose the four cables. Keep the socket end, as the goal is to plug a controller into this cable.
2. Hook up the V+ / VBUS (Red) to the VBUS pin on your Pi Pico
3. Hook up the V- / GND (Black) to ground on your Pi Pico
4. Hook up D+ (Green) to a unused digital pin.
5. Hook up D- (White) to the digital pin directly after D+. For example, you can hook up D+ to GP2 and D- to GP3.

## Configuring
1. Click on 'Add setting'
2. Find and add `USB Host inputs`
3. Bind 'D+'
4. Hit 'Save'
5. If you plug in a supported controller, the tool should detect it and tell you what it is. Most controllers are supported, but accesories like the Xbox 360 Play & Charge Kit won't work over USB.
