---
sort: 2
---
## Rescuing a device that is no longer detected

The tool has the ability to detect devices in firmware update mode, and offer to fix them back up. For all supported arduinos except the Uno, you can just short reset to GND **twice** to get to this mode and it will stay in firmware update mode for a few seconds. As long as the config tool is open, this should be enough time to reprogram the device. There are a few differences to this (Uno and Pico) as shown below, as well as a guide for if this isn't working for a Pro Micro. (A.K.A. - a bad config was pushed)

# Arduino Uno
Short the following pins to enter DFU mode. The tool will recognise the Uno and allow you to program it again.

[![Arduino Uno DFU Pins](/assets/images/ArduinoUnoDFU.svg)](/assets/images/ArduinoUnoDFU.svg)

# Arduino Mega
Short the following pins to enter DFU mode. The tool will recognise the Mega and allow you to program it again.
[![Arduino Mega DFU Pins](/assets/images/ArduinoMegaDFU.svg)](/assets/images/ArduinoMegaDFU.svg)

# Pi Pico
Unplug your Pi Pico from your computer, press and hold the BOOTSEL button and plug it back into your computer while you are holding the button. Once it shows up as a USB drive on your computer, you can stop holding BOOTSEL and program it again with the tool.

[![Pi Pico BOOTSEL](/assets/images/pico-bootsel.png)](/assets/images/pico-bootsel.png)

# Pro Micro 
The Pro Micro is a bit special, in that it will only stay in bootloader mode for 8 seconds. This means that the easiest way to rescue a Pro Micro is to open the tool first.

Once the tool is open, short the rst pin to ground twice in quick succession, and the tool will detect the Pro Micro in bootloader mode. You can then quickly program it again.

[![Pro Micro Reset](/assets/images/promicrorst.png)](/assets/images/promicrorst.png)