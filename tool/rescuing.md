---
sort: 2
---
# Rescuing a device that is no longer detected
The tool has the ability to detect devices in firmware update mode, and offer to fix them back up. Below, are various guides on how to enter firmware update mode on various microcontrollers.

## Arduino Uno
Short the following pins to enter DFU mode. The tool will recognise the Uno and allow you to program it again.

[![Arduino Uno DFU Pins](/assets/images/ArduinoUnoDFU.svg)](/assets/images/ArduinoUnoDFU.svg)

## Arduino Mega
Short the following pins to enter DFU mode. The tool will recognise the Mega and allow you to program it again.
[![Arduino Mega DFU Pins](/assets/images/ArduinoMegaDFU.svg)](/assets/images/ArduinoMegaDFU.svg)

## Pi Pico
Unplug your Pi Pico from your computer, press and hold the BOOTSEL button and plug it back into your computer while you are holding the button. Once it shows up as a USB drive on your computer, you can stop holding BOOTSEL and program it again with the tool.

[![Pi Pico BOOTSEL](/assets/images/pico-bootsel.png)](/assets/images/pico-bootsel.png)

## Pro Micro 
The Pro Micro is a bit special, in that it will only stay in bootloader mode for 8 seconds. This means that the easiest way to rescue a Pro Micro is to open the tool first.

Once the tool is open, short the rst pin to ground twice in quick succession, and the tool will detect the Pro Micro in bootloader mode. You can then quickly program it again.

[![Pro Micro Reset](/assets/images/promicrorst.png)](/assets/images/promicrorst.png)