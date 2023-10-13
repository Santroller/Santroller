# Fakemote
The Guitar Hero games on wii require a guitar that is plugged into a Wii remotes extension port, not a USB instrument.
For this reason, an unmodified console will not be able to support a USB guitar, however the [fakemote](https://github.com/sanjay900/fakemote) CIOS module allows for emulating a wii remote + guitar using a USB instrument.

# Setting up fakemote

## Installing fakemote
This is still a beta tool, but it can be set up using the following instructions:
1. Download `sd_wii.zip` from https://github.com/sanjay900/fakemote/releases/tag/v0.5.0
2. Extract it to the root of your wii's SD card
3. Open the `Homebrew Channel`
4. Launch `d2x cIOS installer (sanjay900)`
5. Press any button to continue when prompted
6. For `Select cIOS` select the last option `v10 sanjay900-53 d2x-v10-sanjay900-53`
7. For `Select cIOS base` make sure it is set to `57`
8. For `Select cIOS slot` make sure it is set to `245`
9. For `Select cIOS revision` make sure it is set to `65535`
10. Press `A`
11. Press `A` again at the next screen
12. Once it has finished installing, hit `B` to return to the homebrew channel.

## Configuring your USB Loader
Most usb loaders will allow for specifying a custom IOS slot when starting a game. For example, the following instructions can be used in `USB Loader GX`:
1. Select the game you want to support USB controllers in
2. Hit `Settings`
3. Hit `Game Load`
4. Scroll down to `Game IOS` and set it to `245`
5. Hit `Save`
6. Hit `Back`
7. Hit `Back`
8. Start the game. This only needs to be done once as the settings will persist.