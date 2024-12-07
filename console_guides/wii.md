# Using Santroller on Wii / Wii U
The Wii is an interesting case, because different games use different types of instruments.

## Rock Band
Rock Band used USB instruments, so if you set your emulation mode to either `Rock Band Guitar` or `Rock Band Drums`, it will just work out of the box.

## Guitar Hero / DJ Hero
Guitar Hero instruments plugged into the Wii Remote. This means we need to emulate the Wii Extension in some form to play these games.

You can either physically wire a Wii extension port to your controller and plug it into a Wii Remote, or you can mod your Wii and use software to emulate a Wii Extension.

## Adding a Wii extension port to your controller
You can follow the [Wii Extension emulation guide](https://santroller.tangentmc.net/wiring_guides/wii_output.html), and actually emulate proper wii extensions, and then plug them into your remote.

## Emulating a Wii Extension in software
I have written a module that you can use with a USB Loader to play Guitar Hero games with a USB instrument, Santroller BSlug. 

### Setting up Santroller BSlug
1. Download `sd.zip` from https://github.com/Santroller/santroller-bslug/releases/latest
2. Extract it to the root of your wii's SD card
3. Rename RGHE52.dol with the title id for the game you wish to play. You can also copy and paste the dol file with multiple names to use this module with multiple different games.
4. Go into USBLoaderGX (other loaders will likley work but instructions will only be provided for USBLoaderGX)
5. Go to loader settings for the game
6. Set `Game IOS` to `Custom`
7. Set `Custom Game IOS` to `249`
8. Set `Alternate dol` to `Load from SD/USB`
9. Save
10. Plug in the USB instrument
11. Launch the game
