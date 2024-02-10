---
sort: 8
---

# Adding RGB LEDs to controllers

You can add LEDs that will light up when a button or analogue input is pressed. For people playing YARG, you can also make the frets light up based on in-game events such as Star Power.

## Supplies

- A controller modded using any of the guides
- Some APA102s (or Dotstar or SK9822 or APA107 or HD107s as these are all compatible / clones of the APA102)
  - Note that if your using JLCPCB, the SK9822 is often available when the APA102 is not.
  - Even though its a clone, the SK9822 is actually a better LED than the APA102. The APA102s will flicker at low brightness, the SK9822 will not.

## The finished product

{% include youtube.html id=H73McH5abes %}

## Steps

1. Connect the VCC and GND pins on all LEDs that are being used to the microcontroller. It is recommended to use the 5V pin for this, as this is what the LEDs expect to be connecfted to. For the Pi Pico, this is the VBUS pin.
2. Connect the SCK (CI) and MOSI (DI) on the first LED to the SCK and MOSI pins on your microcontroller. Then chain the SCK (CO) and MOSI (DO) outputs to the inputs on the next LED, until all LEDs are connected.
   For beginners we provide a recommended pinout below for each microcontroller. These pinouts are the same pinouts that are used on the old Ardwiino firmware, so if you have an old device, this is the pinout it will be using.
   If you are using a Pi Pico and these pins don't work for you, you can use one of the advanced pinouts below. Note that the Pi Pico has multiple channels, and SCK and MOSI need to be from the same channel.

   | Microcontroller               | SCK (CI)         | MOSI (DI)        |
   | ----------------------------- | ---------------- | ---------------- |
   | Pi Pico (Recommended)         | GP6              | GP3              |
   | Pro Micro, Leonardo, Micro    | 15               | 16               |
   | Uno                           | 13               | 11               |
   | Mega                          | 52               | 51               |
   | Pi Pico (Advanced, Channel 0) | GP2, GP6, GP18   | GP3, GP7, GP19   |
   | Pi Pico (Advanced, Channel 1) | GP10, GP14, GP26 | GP11, GP15, GP27 |

Now that you have wired your LEDs, go [configure them](https://santroller.tangentmc.net/tool/using.html).
