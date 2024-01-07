---
sort: 9
---
# Wiring USB Inputs (Pi Pico Only)
## Supplies
* A Pi Pico
* A USB Extension cable, or a USB breakout

## Wiring Steps
1. If you are using a USB extension cable, cut it in half and expose the four cables.
2. Hook up the V+ (Red) to the VBUS pin on your Pi Pico
3. Hook up the V- (Black) to ground on your Pi Pico
4. Hook up D+ (Green) to a unused digital pin.
5. Hook up D- (White) to the digital pin directly after D+. For example, you can hook up D+ to GP2 and D- to GP3.

## Configuring
1. Click on Add setting
2. Find and add `USB Host inputs`
3. Bind D+
4. Hit Save
5. If you plug in a supported controller, the tool should detect it and tell you what it is.


Now that you have USB Inputs wired, you can [configure them](https://santroller.tangentmc.net/tool/using.html).
