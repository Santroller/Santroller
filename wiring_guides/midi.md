---
sort: 7
---
# Setting up an Adaptor for a MIDI device (Pro Keys / Drums) (Pi Pico Only)
## Supplies
* A Pi Pico
* A USB Extension cable, or a USB breakout

## Do i need to keep the controller plugged in
For Xbox 360, Xbox One, no, you should be able to unplug your controller after authentication is done.

## Wiring Steps

1. If you are using a USB extension cable, cut it in half and expose the four cables. Keep the socket end, as the goal is to plug a controller into this cable.
2. Hook up the V+ / VBUS (Red) to the VBUS pin on your Pi Pico
3. Hook up the V- / GND (Black) to ground on your Pi Pico
4. Hook up D+ (Green) to a unused digital pin.
5. Hook up D- (White) to the digital pin directly after D+. For example, you can hook up D+ to GP2 and D- to GP3.

## Configuring
1. Click on Add setting
2. Find and add `USB Host inputs`
3. Bind D+
4. Hit Save
5. Click on Add setting
6. Find and add `MIDI inputs`
7. Hit Save
7. For pro keys, you will need to pick what key your keyboard starts at. You can hit `Click to assign` and then press a note on your keyboard, and that will be mapped to the Pro Keys note, with every other note being assigned to the next notes in order.
8. If you plug in a supported controller, the tool should detect it and tell you what it is. Most controllers are supported, but things like the play and charge kit won't work over USB.