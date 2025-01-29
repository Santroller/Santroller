---
sort: 9
---

# Adding WS2812 RGB LEDs to controllers

You can add LEDs that will light up when a button or analogue input is pressed. For people playing YARG, you can also make the frets light up based on in-game events such as Star Power.

```danger
This is only supported on the Pi Pico, as it has hardware onboard that can drive these LEDs without needing the processor to drive them manually.
```

## Supplies

- A Pi Pico (or Pi Pico 2, or any controller based on them)
- A controller modded using any of the guides
- Some WS2812s (or SK6812)
  - Even though its a clone, the SK6812 is actually a better LED than the WS2812.

## Steps

1. Connect the V<sub>CC</sub> and GND pins on all LEDs that are being used to the microcontroller. It is recommended to use the 5V pin for this, as this is what the LEDs expect to be connected to. For the Pi Pico, this is the VBUS pin.
2. Connect the data on the first LED to any pin on the Pi Pico. Then chain the data outputs to the data on the next LED, until all LEDs are connected.
   For beginners we provide a recommended pinout below for each microcontroller.

## Programming

1. Click on LED Settings
2. Set the LED Type to WS2812
3. Set the data pin as you wired it
4. Set the LED count to the amount of LEDs you have connected together.
5. Hit `Save Settings`
6. If you want a button to control an LED, click on the button in question.
7. Click on the leds you want to link to this input in the `Selected LEDs` section.
8. You can then set the colour when the button is pressed, and the colour when it is released.
9. As you pick a colour the LED should update in real time so you can preview what the colour looks like.
10. Note that the WS2812 LEDs don't support global brightness, so brightness control is done by setting a darker colour. This can easily be done by changing the colour mode to `HSV`, and the `V` will act as a brightness slider for a given colour.
11. If you want an analog input (such as whammy) to control an LED, click on it
12. Configuring analog based LEDs is much the same as digital, but instead of pressed and released being digital, the LEDs will smoothly transition between pressed and released depending on the value of the analog input.
13. If you would like to respond to game events in supported games, hit the `Add Setting` button and add an `LED Binding`. You can then pick the LED type in question, and configure it, in much the same way as the inputs.

## LED inactivity

You can turn on the `Turn on LEDs after inactivity` setting to have the leds turn completly off after a timer. This is useful if you have leds on `always on` mode, but wish to have your guitar power its LEDs off after it is unused for a given amount of time.
