---
sort: 9
---

# Adding WS2812 RGB LEDs to controllers

You can add LEDs that will light up when a button or analogue input is pressed. For people playing YARG, you can also make the frets light up based on in-game events such as Star Power.

We abuse the SPI hardware on the microcontroller to get around some problems that previously stopped us implementing WS2812 support in the past. This means that the WS2812s don't have any problems, but it does mean that only certain pins can be used for driving the WS2812, much like how the APA102s work.

## Supplies

- A controller modded using any of the guides
- Some WS2812s (or SK6812)
  - Even though its a clone, the SK6812 is actually a better LED than the WS2812.

## Steps

1. Connect the V<sub>CC</sub> and GND pins on all LEDs that are being used to the microcontroller. It is recommended to use the 5V pin for this, as this is what the LEDs expect to be connected to. For the Pi Pico, this is the VBUS pin.
2. Connect the data on the first LED to the MOSI pins on your microcontroller. Then chain the data outputs to the data on the next LED, until all LEDs are connected.
   For beginners we provide a recommended pinout below for each microcontroller. 
   If you are using a Pi Pico and these pins don't work for you, you can use one of the advanced pinouts below.

   | Microcontroller               | MOSI (data)        |
   | ----------------------------- | ---------------- |
   | Pi Pico (Recommended)         | GP3              |
   | Pro Micro, Leonardo, Micro    | 16               |
   | Uno                           | 11               |
   | Mega                          | 51               |
   | Pi Pico (Advanced, Channel 0) | GP3, GP7, GP19   |
   | Pi Pico (Advanced, Channel 1) | GP11, GP15, GP27 |

## Programming

1. Click on LED Settings
2. Set the LED Type to WS2812
3. Set the data pin as you wired it
5. Set the LED count to the amount of LEDs you have connected together.
8. Hit `Save Settings`
7. If you want a button to control an LED, click on the button in question. 
8. Click on the leds you want to link to this input in the `Selected LEDs` section.
9. You can then set the colour when the button is pressed, and the colour when it is released.
10. As you pick a colour the LED should update in real time so you can preview what the colour looks like.
11. Note that the WS2812 LEDs don't support global brightness, so brightness control is done by setting a darker colour. This can easily be done by changing the colour mode to `HSV`, and the `V` will act as a brightness slider for a given colour.
12. If you want an analog input (such as whammy) to control an LED, click on it
13. Configuring analog based LEDs is much the same as digital, but instead of pressed and released being digital, the LEDs will smoothly transition between pressed and released depending on the value of the analog input.
14. If you would like to respond to game events in supported games, hit the `Add Setting` button and add an `LED Binding`. You can then pick the LED type in question, and configure it, in much the same way as the inputs.