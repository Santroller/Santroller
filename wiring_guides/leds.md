---
sort: 9
---

# Adding SK9822 or APA102 RGB LEDs to controllers

You can add LEDs that will light up when a button or analogue input is pressed. For people playing YARG, you can also make the frets light up based on in-game events such as Star Power.

## Supplies

- A controller modded using any of the guides
- Some SK9822s (or Dotstar or APA102 or APA107 or HD107s as these are all compatible / clones of the APA102)
  - Note that the SK9822 is a clone of the APA102, but it uses a constant current driver so it functions better at low brightnesses and it is more available.

## The finished product

{% include youtube.html id=H73McH5abes %}

## Steps

1. Connect the V<sub>CC</sub> and GND pins on all LEDs that are being used to the microcontroller. It is recommended to use the 5V pin for this, as this is what the LEDs expect to be connected to. For the Pi Pico, this is the VBUS pin.
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

## Programming

1. Click on LED Settings
2. Set the LED Type to one of the APA102 settings. If you know which LED type you are using, you can pick that here, otherwise try `APA102 - RBG`.
3. Set the CI and DI pins as you wired them
4. Set the LED Brightness. For full sized APA102s, you can use max brightness, but it is recommended to set the brightness for the smaller 2020 leds to 9 or lower.
5. Set the LED count to the amount of LEDs you have connected together.
8. Hit `Save Settings`
7. If you want a button to control an LED, click on the button in question. 
8. Click on the leds you want to link to this input in the `Selected LEDs` section.
9. You can then set the colour when the button is pressed, and the colour when it is released.
10. As you pick a colour the LED should update in real time so you can preview what the colour looks like. If the colour is totally wrong, you likley picked the wrong APA102 type, and will need to change the LED Type to figure out the correct order for your specific LEDs.
11. If you want an analog input (such as whammy) to control an LED, click on it
12. Configuring analog based LEDs is much the same as digital, but instead of pressed and released being digital, the LEDs will smoothly transition between pressed and released depending on the value of the analog input.
13. If you would like to respond to game events in supported games, hit the `Add Setting` button and add an `LED Binding`. You can then pick the LED type in question, and configure it, in much the same way as the inputs.
