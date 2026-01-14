# Authentication controllers
Some consoles check if a controller is a genuine controller. Our controllers are unoffical, so we need to trick the console into thinking our controller is genuine.

There are a few ways you can use an unofficial controller on a console requiring authentication.
1. Pass through the authentication check to another controller or dongle.
2. Patch out authentication as a whole, as with [UsbdSecPatch](https://github.com/InvoxiPlayGames/UsbdSecPatch)
3. Some games support input methods that don't need authentication, as on Rock Band 4 on PS4 supporting PS3 instruments or Fortnite on Xbox and PS4/5 supporting keyboard inputs.

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
    - For Fortnite Festival, you can follow the [Fortnite Festival](https://santroller.tangentmc.net/tool/fortnite_festival.html) guide.
    - For Rock Band 4, you can use PS3 mode. Do note however that when you do this, it will only work in game, and not in the PS5 system menu.
    - For any other game, you require the [Besavior P5General](https://p5general.com).
## Passing authentication checks to a legitimate controller
You can follow the [USB Inputs](https://santroller.tangentmc.net/wiring_guides/usb.html) guide. When USB Inputs are enabled, authentication checks will automatically be passed to a controller.

## Supported controllers

Xbox One / Series S|X: The Xbox One or Series gamepads will work here, third party controllers do not. 

PS4: DualShock 4 and most other controllers should work. Dongles such as the [MAGPS4](https://www.mayflash.com/product/MAGPS4.html) should also work for this.

Native PS5: A [Besavior P5General](https://p5general.com) is required for use on PS5 for all games. The Magicboots-S5 will NOT work for this. 

