# Authentication controllers
Some consoles check if a controller is a genuine controller. Our controllers are unoffical, so we need to trick the console into thinking our controller is genuine.

There are currently three ways to trick the console
1. Pass the check to an offical controller, by plugging a genuine controller into your Pi Pico. (This method only works on the Pi Pico)
2. Mod the console. There is a patch for the Xbox 360 that removes the authentication check.
3. Some games support controllers that don't need authentication, such as a keyboard, or a PS3 instrument.

## Consoles needing authentication
- Xbox 360
    - The Xbox 360 in most cases requires authentication, but Santroller has authentication keys built in. 
- Xbox One and Series S|X, but only if you are not playing Fortnite Festival. 
    - For Fortnite Festival, you can follow the [Fortnite Festival](https://santroller.tangentmc.net/tool/fortnite_festival.html) guide.
- PS4
    - For Rock Band 4, you can use PS3 mode. Do note however that when you do this, it will only work in game, and not the PS4 system menu.
    - For Rock Band 4 and GHL, you can patch the game to support other instruments using [OrbisInstrumentalizer](https://github.com/InvoxiPlayGames/OrbisInstrumentalizer) if you have a GoldHEN modded PS4.
    - For Fortnite Festival, you can follow the [Fortnite Festival](https://santroller.tangentmc.net/tool/fortnite_festival.html) guide.
- PS5
    - For Fortnite Festival, you can follwo the [Fortnite Festival](https://santroller.tangentmc.net/tool/fortnite_festival.html) guide.
    - For Rock Band 4, you can use PS3 mode. Do note however that when you do this, it will only work in game, and not in the PS5 system menu.
    - For any other game, you require 
## Passing authentication checks to a legitimate controller
You can follow the [USB Inputs](https://santroller.tangentmc.net/wiring_guides/usb.html) guide. When USB Inputs are enabled, authentication checks will automatically be passed to a controller.

## Supported controllers

Xbox One / Series S|X: The Xbox One or Series gamepads will work here, third party controllers do not. 

PS4: DualShock 4 and most other controllers should work. Dongles such as the [MAGPS4](https://www.mayflash.com/product/MAGPS4.html) should also work for this.

Native PS5: A [Besavior P5General](https://p5general.com) is required for use on PS5 for all games. 

