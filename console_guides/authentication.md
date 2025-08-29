# Authentication controllers
Some consoles check if a controller is a genuine controller. Our controllers are unoffical, so we need to trick the console into thinking our controller is genuine.

There are currently three ways to trick the console
1. Pass the check to an offical controller, by plugging a genuine controller into your Pi Pico. (This method only works on the Pi Pico)
2. Mod the console. There is a patch for the Xbox 360 that removes the authentication check.
3. Some games support controllers that don't need authentication, such as a keyboard, or a PS3 instrument.

## Consoles needing authentication
- Xbox 360
    - Usually needs authentication in form of a wired controller, but as of v10.4.0, the Santroller firmware now has authentication for unmodded Xbox 360s built in. Only works on the Pi Pico, otherwise you need a modded console.
- Xbox One and Series S|X, but only if you are not playing Fortnite Festival. 
    - For Fortnite Festival you can follow the [Fortnite Festival](https://santroller.tangentmc.net/tool/fortnite_festival.html) guide.
- PS4 / PS5, but only if you are not playing Fortnite Festival or Rock Band 4. 
    - For Rock Band 4, you can use PS3 mode. Do note however that when you do this, it will only work in game, and not the PS4/5 system menu.
    - For Rock Band 4 and GHL, you can patch the game to support other instruments using [OrbisInstrumentalizer](https://github.com/InvoxiPlayGames/OrbisInstrumentalizer) if you have a GoldHEN modded PS4.
    - For Fortnite Festival you can follow the [Fortnite Festival](https://santroller.tangentmc.net/tool/fortnite_festival.html) guide.

## Passing authentication checks to a legitimate controller
You can follow the [USB Inputs](https://santroller.tangentmc.net/wiring_guides/usb.html) guide. When USB Inputs are enabled, authentication checks will automatically be passed to a controller.

## Supported controllers

Xbox One / Series S|X: The Xbox One or Series gamepads will work here, third party controllers do not. 

PS4: DualShock 4 works, most other controllers should work too.

PS5: A "typed" controller is needed. A GHLive dongle or a [MAGPS4](https://www.mayflash.com/product/MAGPS4.html) should work for this, or a PS4 drumkit, guitar, wheel or fight stick. A Dualshock 4 or Dualsense will NOT work here.

