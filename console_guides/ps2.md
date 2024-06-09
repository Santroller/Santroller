# PS2 Support

| Controller Type                        | Game                    | PS2 Support                                                                                                                                                                                                      |
| -------------------------------------- | ----------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Gamepad                                | Any                     | Requires [PS2 Controller emulation](https://santroller.tangentmc.net/wiring_guides/ps2_output.html) or a Modded PS2 + [nightly build of OPL](https://github.com/ps2homebrew/Open-PS2-Loader/releases/tag/latest) |
| Guitar Hero Guitar                     | Guitar Hero 2           | Requires [PS2 Controller emulation](https://santroller.tangentmc.net/wiring_guides/ps2_output.html) or a Modded PS2 + [nightly build of OPL](https://github.com/ps2homebrew/Open-PS2-Loader/releases/tag/latest) |
| Guitar Hero Guitar                     | Guitar Hero 3           | Requires [PS2 Controller emulation](https://santroller.tangentmc.net/wiring_guides/ps2_output.html) or a Modded PS2 + [nightly build of OPL](https://github.com/ps2homebrew/Open-PS2-Loader/releases/tag/latest) |
| Guitar Hero Guitar / Guitar Hero Drums | Guitar Hero WT or newer | Works natively                                                                                                                                                                                                   |
| Rock Band Guitar / Rock Band Drums     | Rock Band (All Games)   | Works natively                                                                                                                                                                                                   |
| DJ Hero Turntable                      | DJ Hero                 | Works natively                                                                                                                                                                                                   |

PS2 controller support is totally dependant on the game and controller type. For games that expect standard gamepad input, you will need to use PADEMU to emulate a standard controller, as the PS2 did not support USB controllers.
For Rock Band games, the PS2 used USB instruments, so a controller in Rock Band Guitar or Drum mode will just work.

Guitar Hero games are a bit more complicated. The first two games used a guitar that was based on the Dualshock 1, which used the standard PS2 controller port. For this reason, these games don't support USB instruments. PADEMU will automatically detect PS3 guitars and emulate PS2 Guitars for these games if it is enabled.
Guitar Hero World Tour introduced compatibility with PS3 instruments and thus for GHWT or newer, you can just use USB instruments without needing something like PADEMU.

## Emulating a PS2 controller

You can follow the [PS2 controller guide](https://santroller.tangentmc.net/wiring_guides/ps2_output.html), and actually emulate proper PS2 controllers.

## PADEMU

If a PS2 game is expecting a standard controller you usually would not be able to use a USB controller. However, OpenPS2Loader comes with PADEMU, which lets you emulate a PS2 gamepad from a usb device.
The firmware is designed to go into PS3 mode when it detects it is plugged into a PS2, and PADEMU has support for both PS3 gamepads and PS3 guitars.
Do note that this method will NOT work for offical game discs, as OPL does not support booting discs, only disc images stored on the internal HDD or a network share or a USB drive.

## Setting up PADEMU

1. Plug the santroller controller into a USB port
1. Plug your standard PS2 gamepad into port 2
1. Make sure you have a [nightly build of OPL](https://github.com/ps2homebrew/Open-PS2-Loader/releases/tag/latest)
1. Launch OPL
1. Go into `Controller Settings`
1. Go into `Configure PADEMU`
1. Turn on `Pad Emulator`
1. Set `Pad Emulator Mode` to `DualShocker3/4 USB`
1. Set `Multitap Emulation` to `Off`
1. Set `Settings For Port` to `1P`
1. Set `Emulation` to `On`
1. Click `OK`
1. Click `Save Changes`
1. Start the game from within OPL
1. Your Santroller device should now be working in game.
