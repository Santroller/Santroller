# Authentication controllers
Some consoles require controller authentication, such as the Xbox 360 and the Xbox One / Series. For these consoles, you will either need to mod the authentication checks away, or follow the [USB](https://santroller.tangentmc.net/wiring_guides/usb.html) guide, and then plug in an offical controller for authentication.

## Consoles needing authentication
- Xbox 360, though this can be patched away using [UsbdSecPatch](#usbdsecpatch)
- Xbox One and Series
- PS4 / PS5, but only when emulating a gamepad or a Guitar Hero Live guitar. For other instruments, we emulate PS3 instruments which do not require auth.

## UsbdSecPatch
You can use [UsbdSecPatch](https://github.com/InvoxiPlayGames/UsbdSecPatch/releases) to patch out the authentication checks on an RGH modded console.

## Instructions:
1. Mod your console and install dashlaunch
2. Copy the xex file from above to your console
3. Add it as a plugin to dashlaunch
4. Unplug your controller, and plug it in AFTER the boot animation.