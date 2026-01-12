# Using Santroller on Wii / Wii U

The Wii is an interesting case because different games use different types of instruments.

---

## Rock Band

Rock Band uses USB instruments. If you set your emulation mode to either `Rock Band Guitar` or `Rock Band Drums`, it will work out of the box.

---

## Guitar Hero / DJ Hero

Guitar Hero and DJ Hero instruments connect through the Wii Remote rather than USB. Because of this, you must emulate a Wii Extension in order to play these games.

There are two ways to do this:

- Physically wire a Wii Extension port to your controller and plug it into a Wii Remote
- Mod your Wii and use software to emulate a Wii Extension

---

## Adding a Wii Extension port to your controller

You can follow the [Wii Extension emulation guide](https://santroller.tangentmc.net/wiring_guides/wii_output.html) to emulate proper Wii Extensions and plug them directly into a Wii Remote.

---

## Emulating a Wii Extension in software

A module is available that works with USB loaders and allows you to play Guitar Hero, Band Hero, and DJ Hero games using a USB instrument. This module is called **Santroller Brainslug**.

---

### Setting up Santroller Brainslug (Real Disc)

1. Download `sd.zip` from https://github.com/Santroller/santroller-bslug/releases/latest
2. Extract the contents to the root of your Wii SD card
3. Plug in the USB instrument
4. Launch the Homebrew Channel
5. Launch Brainslug

---

### Setting up Santroller Brainslug (USB Loader GX)

1. Download `sd.zip` from https://github.com/Santroller/santroller-bslug/releases/latest
2. Extract the contents to the root of your Wii SD card
3. Rename `RGHE52.dol` to the title ID of the game you want to play
   - You may copy and rename the file multiple times to support multiple games
4. Open USBLoaderGX
   - Other loaders may work, but instructions are only provided for USBLoaderGX
5. Open the loader settings for the game
6. Set `Game IOS` to `Custom`
7. Set `Custom Game IOS` to `249`
8. Set `Alternate dol` to `Load from SD/USB`
9. Save the settings
10. Plug in the USB instrument
11. Launch the game

