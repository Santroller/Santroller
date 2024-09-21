---
sort: 7
---
# Setting up an Adaptor for a USB controller (Pi Pico Only)
## Supplies
* A Pi Pico
* A USB Extension cable, or a USB breakout
* 3.3V to 5V boost converter 
   * only necessary if you wish to use USB host while plugging your pico into a Wii remote or a ps2 controller port
   * This is not necessary if you are using the adafruit feather with USB host, as that board has one built in.

## Do i need to keep the controller plugged in
For Xbox 360 and Xbox One no, you should be able to unplug your controller after authentication is done.
For PS4/PS5, you need to leave it plugged in, playstations continuously check the authentication state.

## Wiring Steps

1. If you are using a USB extension cable, cut it in half and expose the four cables. Keep the socket end, as the goal is to plug a controller into this cable.
2. Hook up the V+ / VBUS (Red) to the VBUS pin on your Pi Pico
3. Hook up the V- / GND (Black) to ground on your Pi Pico
4. Hook up D+ (Green) to a unused digital pin.
5. Hook up D- (White) to the digital pin directly after D+. For example, you can hook up D+ to GP2 and D- to GP3.

## Wiring for USB Host + PS2 or Wii emulation
1. Follow one of the above guides for GND, D+ and D-
2. Connect GND on the Pico to GND on the boost converter
3. Connect VSYS on the Pico to the input terminal on your boost converter
4. Connect the V+ / VBUS (Red) from your breakout or extension to the output pin on your boost converter

## Configuring
1. Click on Add setting
2. Find and add `USB Host inputs`
3. Bind D+
4. Hit Save
5. If you plug in a supported controller, the tool should detect it and tell you what it is. Most controllers are supported, but things like the play and charge kit won't work over USB.
