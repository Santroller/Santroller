# Authentication controllers
Some consoles check if a controller is a genuine controller. Our controllers are unoffical, so we need to trick the console into thinking our controller is genuine.

There are currently two ways to trick the console
1. Pass the check to an offical controller, by plugging a genuine controller into your Pi Pico. (This method only works on the Pi Pico)
2. Mod the console. There is a patch for the Xbox 360 that removes the authentication check.

## Consoles needing authentication
- Xbox 360, though this can be patched away using [UsbdSecPatch](#usbdsecpatch) on RGHed consoles
- Xbox One and Series
- PS4 / PS5, but only when emulating a gamepad or a Guitar Hero Live guitar. For other instruments, we emulate PS3 instruments which do not require auth.

## Passing authentication checks to a legitimate controller
You can follow the [USB Inputs](https://santroller.tangentmc.net/wiring_guides/usb.html) guide. When USB Inputs are enabled, authentication checks will automatically be passed to a controller.

## Xbox 360 RGH
You can use [UsbdSecPatch](https://github.com/InvoxiPlayGames/UsbdSecPatch/releases) module to patch out the authentication checks on an RGH modded console.

### Instructions:
1. Mod your console and install dashlaunch
2. Copy the xex file from above to your console
3. Add it as a plugin to dashlaunch
4. Unplug your controller, and plug it in AFTER the boot animation.