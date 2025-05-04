---
sort: 7
---
# Setting up an Adaptor for a USB MIDI device (Pro Keys / Drums) (Pi Pico Only)
## Supplies
* A Raspberry Pi Pico
* A USB Extension cable, or a USB breakout

## Do I need to keep the controller plugged in?
For Xbox 360 and Xbox One, no. You are able to disconnect the controller once it is authenticated.

## Wiring Steps

1. If you are using a USB extension cable, cut it to and expose the four cables. Keep the socket end, as the goal is to plug a controller into this cable.
2. Hook up the V+ / VBUS (Red) to the VBUS pin on your Pi Pico
3. Hook up the V- / GND (Black) to ground on your Pi Pico
4. Hook up D+ (Green) to a unused digital pin.
5. Hook up D- (White) to the digital pin directly after D+. For example, you can hook up D+ to GP2 and D- to GP3.

## Configuring
1. Click on `Add Setting` on left of Santroller Window
2. Find and add `USB Host inputs`
3. Bind `D+`
4. Click `Save`
5. Click on `Add setting`
6. Find and add `MIDI inputs`
7. Click `Save`
7. For Pro Keys, you will need to pick what key your keyboard starts at. You can click on `Click to assign`, then press a note on your keyboard. That will be mapped to the Pro Keys note, with every other note being assigned to the next notes in order.
8. If you plug in a supported controller, the tool should detect it and tell you what it is. Most controllers are supported, but things like the play and charge kit won't work over USB.