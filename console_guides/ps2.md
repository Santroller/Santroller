# PS2 Support

| Controller Type                        | Game                    | PS2 Support                                                                               | 
| -------------------------------------- | ----------------------- | ----------------------------------------------------------------------------------------- | 
| Gamepad                                | Any                     | Requires [nightly build of OPL](https://santroller.tangentmc.net/console_guides/ps2.html) | 
| Guitar Hero Guitar                     | Guitar Hero 2           | Requires [nightly build of OPL](https://santroller.tangentmc.net/console_guides/ps2.html) | 
| Guitar Hero Guitar                     | Guitar Hero 3           | Requires [nightly build of OPL](https://santroller.tangentmc.net/console_guides/ps2.html) | 
| Guitar Hero Guitar / Guitar Hero Drums | Guitar Hero WT or newer | Works natively                                                                            | 
| Rock Band Guitar / Rock Band Drums     | Rock Band (All Games)   | Works natively                                                                            | 
| DJ Hero Turntable                      | DJ Hero                 | Works natively                                                                            | 

PS2 controller support is totally dependant on the game and controller type. For games that expect standard gamepad input, you will need to use PADEMU to emulate a standard controller, as the PS2 did not support USB controllers.
For Rock Band games, the PS2 used USB instruments, so a controller in Rock Band Guitar or Drum mode will just work.

Guitar Hero games are a bit more complicated. The first two games used a guitar that was based on the Dualshock 1, which used the standard PS2 controller port. For this reason, these games don't support USB instruments. PADEMU will automatically detect PS3 guitars and emulate PS2 Guitars for these games if it is enabled.
Guitar Hero World Tour introduced compatibility with PS3 instruments and thus for GHWT or newer, you can just use USB instruments without needing something like PADEMU.

## PADEMU

If a PS2 game is expecting a standard controller you usually would not be able to use a USB controller. However, OpenPS2Loader comes with PADEMU, which lets you emulate a PS2 gamepad from a usb device.
The firmware is designed to go into PS3 mode when it detects it is plugged into a PS2, which PADEMU will recognise. Note that for guitars, you need to use a [development relase of OPL](https://github.com/ps2homebrew/Open-PS2-Loader/releases/download/latest/OPNPS2LD.7z), as I added support for guitar emulation myself as it originally was not part of PADEMU.
