# Authentication controllers
Some consoles check if a controller is a genuine controller. Our controllers are unoffical, so we need to trick the console into thinking our controller is genuine.

There are currently three ways to trick the console
1. Pass the check to an offical controller, by plugging a genuine controller into your Pi Pico. (This method only works on the Pi Pico)
2. Mod the console. There is a patch for the Xbox 360 that removes the authentication check.
3. Some games support controllers that don't need authentication, such as a keyboard, or a PS3 instrument.

## Consoles needing authentication
- Xbox 360
    - You can authenticate with a stock, unmodded console by dumping the NAND and keys using [Xbox360BadUpdate](#xbox-360-badupdate)
    - This can be patched out using [UsbdSecPatch](#xbox-360-rgh) on RGH or JTAG modded consoles, or after running BadUpdate.
- Xbox One and Series S|X, but only if you are not playing Fortnite Festival. 
    - For Fortnite Festival you can follow the [Fortnite Festival](https://santroller.tangentmc.net/tool/fortnite_festival.html) guide.
- PS4 / PS5, but only if you are not playing Fortnite Festival or Rock Band 4. 
    - For Rock Band 4, you can use PS3 mode. Do note however that when you do this, it will only work in game, and not the PS4/5 system menu.
    - For GHL, you can patch the game to support other instruments using [OrbisInstrumentalizer](https://github.com/InvoxiPlayGames/OrbisInstrumentalizer) if you have a GoldHEN modded PS4.
    - For Fortnite Festival you can follow the [Fortnite Festival](https://santroller.tangentmc.net/tool/fortnite_festival.html) guide.

## Passing authentication checks to a legitimate controller
You can follow the [USB Inputs](https://santroller.tangentmc.net/wiring_guides/usb.html) guide. When USB Inputs are enabled, authentication checks will automatically be passed to a controller.

## Supported controllers
Xbox 360: most wired controllers should work.

Xbox One / Series S|X: The Xbox One or Series gamepads will work here, third party controllers do not. 

PS4: DualShock 4 works, most other controllers should work too.

PS5: A "typed" controller is needed. A GHLive dongle or a [MAGPS4](https://www.mayflash.com/product/MAGPS4.html) should work for this, or a PS4 drumkit, guitar, wheel or fight stick. A Dualshock 4 or Dualsense will NOT work here.

## Xbox 360 BadUpdate
You can use the [Xbox360BadUpdate](https://github.com/Grimdoomer/Xbox360BadUpdate) exploit to dump the keys needed to authenticate a controller with your console.

### Instructions
1. Follow [MrMario2011's video guide](https://www.youtube.com/watch?v=3Ay0V2edQJU) on setting up Xbox360BadUpdate and dumping your NAND and CPU key.
    * Make sure to completely disconnect your Xbox from the internet.
3. In the Santroller Configurator, in "Controller Settings" click "Import Keys from NAND Image".
4. Select the flashdmp.bin file on your USB drive, dumped in the tutorial linked above. If asked, find your cpukey.txt.
5. Make sure the ID shown matches the Console ID shown in your Xbox 360's System Info.
6. Click "Save Settings" to write the new firmware with keys back to your controller.
7. **Make sure to reboot your Xbox console before connecting it back to the internet.**
8. Note that you can easily export your keys with the "Export Keys to Backup" and then use the "Import Keys from Backup" if you wish to move all your keys between devices.

## Xbox 360 RGH
You can use [UsbdSecPatch](https://github.com/InvoxiPlayGames/UsbdSecPatch/releases) module to patch out the authentication checks on an RGH modded console.

### Instructions:
1. Mod your console and install [DashLaunch](https://consolemods.org/wiki/File:DashLaunch_v3.21.7z)
2. Copy the .xex file you downloaded to your Xbox 360.
3. Add [UsbdSecPatch](https://github.com/InvoxiPlayGames/UsbdSecPatch/releases) to your [DashLaunch](https://consolemods.org/wiki/File:DashLaunch_v3.21.7z) plugins.
4. Unplug your controller, and plug it in after the boot animation.
