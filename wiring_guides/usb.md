---
sort: 7
---
# Setting up an Adaptor for a USB controller (Pi Pico Only)
## Supplies
* A Pi Pico
* A USB Extension cable, or a USB breakout
* 2x 27ohm resistor (optional, recommended if you want to use Hubs or PS3 instruments)
* 2x 47pF capacitor (optional, recommended if you want to use Hubs or PS3 instruments)

## Do I need the capacitor and resistor

If you are going to be selling your devices, its highly recommended that you include these, as some USB devices do require them.
However, if you are making this strictly for authentication reasons you likley won't need them, as most gamepads will work without them.

## Do i need to keep the controller plugged in
For Xbox 360, Xbox One and Festival on PC, no, you should be able to unplug your controller after authentication is done.
For PS4/PS5, you need to leave it plugged in, playstations continuously check the authentication state.

## Wiring Steps (with capacitor and resistor)

[![usb](/assets/images/usb.png)](/assets/images/usb.png)

1. If you are using a USB extension cable, cut it in half and expose the four cables.
2. Hook up the V+ / VBUS (Red) to the VBUS pin on your Pi Pico
3. Hook up the V- / GND (Black) to ground on your Pi Pico
4. Hook up D+ (Green) to one side of a 27ohm resistor and the other side to a unused digital pin.
5. Hook up D- (White) to one side of a 27ohm resistor and the other side to the digital pin directly after D+. For example, you can hook up D+ to GP2 and D- to GP3.
6. Connect a 47pF capacitor between D+ and ground
7. Connect a 47pF capacitor between D- and ground

## Wiring Steps (without capacitor and resistor)

[![usb](/assets/images/usb.png)](/assets/images/usb.png)

1. If you are using a USB extension cable, cut it in half and expose the four cables.
2. Hook up the V+ / VBUS (Red) to the VBUS pin on your Pi Pico
3. Hook up the V- / GND (Black) to ground on your Pi Pico
4. Hook up D+ (Green) to a unused digital pin.
5. Hook up D- (White) to the digital pin directly after D+. For example, you can hook up D+ to GP2 and D- to GP3.

## Configuring
1. Click on Add setting
2. Find and add `USB Host inputs`
3. Bind D+
4. Hit Save
5. If you plug in a supported controller, the tool should detect it and tell you what it is. Note that only wired controllers will work for this, wireless only controllers or the play and charge kit won't work.