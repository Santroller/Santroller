---
sort: 7
---
# Setting up an Adaptor for a USB controller (Pi Pico Only)
## Supplies
* A Pi Pico
* A USB Extension cable, or a USB breakout
* 2x 27ohm resistor
* 2x 47pF capacitor

## Wiring Steps

[![usb](/assets/images/usb.png)](/assets/images/usb.png)

1. If you are using a USB extension cable, cut it in half and expose the four cables.
2. Hook up the V+ / VBUS (Red) to the VBUS pin on your Pi Pico
3. Hook up the V- / GND (Black) to ground on your Pi Pico
4. Hook up D+ (Green) to one side of a 27ohm resistor and the other side to a unused digital pin.
5. Hook up D- (White) to one side of a 27ohm resistor and the other side to the digital pin directly after D+. For example, you can hook up D+ to GP2 and D- to GP3.
6. Connect a 47pF capacitor between D+ and ground
7. Connect a 47pF capacitor between D- and ground

## Configuring
1. Click on Add setting
2. Find and add `USB Host inputs`
3. Bind D+
4. Hit Save
5. If you plug in a supported controller, the tool should detect it and tell you what it is. Note that only wired controllers will work for this, wireless only controllers or the play and charge kit won't work.