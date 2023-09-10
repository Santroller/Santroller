# Building an Adaptor for a PS2 Controller
## You will need
* One of the following Arduinos:
  * Pi Pico (I recommend this the most, as it is the easiest to get started with and for most people it will be easier to purchase, and it runs at 3.3v so it works for everything.)
  * Arduino Pro Micro (3.3v)
  * With the following devices, you will need the following level shifter: https://learn.sparkfun.com/tutorials/bi-directional-logic-level-converter-hookup-guide/all
    * Arduino Leonardo
    * Arduino Micro
    * Arduino Uno
    * Arduino Mega
    * You can also use a 5v Pro Micro, but you will also need a 3.3v voltage regulator
```danger
  The arduino nano is NOT supported as it does not have the necessary hardware onboard for emulating a usb device. Some very very cheap arduino unos will also have this problem as they replace the programmable usb controller with a non programmable serial adapter. I don't recommend UNOs at all, but if your arduino uno mentions anything about a ch340g it won't work.
```
* PS2 Controller socket or some male dupont wires
* 2x 1kΩ Resistor
* Some Wire
* A Soldering Iron

```note
If you are unsure what microcontroller you want to use, click [![here](https://sanjay900.github.io/guitar-configurator/guides/micro-controller-comparison.html){: width="250" }](https://sanjay900.github.io/guitar-configurator/guides/micro-controller-comparison.html) for a list of pros and cons about each microcontroller.
```
```danger
note that on the pi pico you need to use the `3v3 out` pin (pin 36)  for your VCC, not 5V or `3v3_en`. The pins on the pico are not rated for 5v, and the `3v3_en` pin is actually an input that will stop your pico from starting.
```
```danger
Be careful that you don't ever provide 5v power to any pins on the PS2 Controller. While some people have done this successfully and just use their controllers with 5v, I can't recommend this approach as the controllers are really designed for 3.3v, so it may damage them slowly over time.
```

```danger
Be careful when programming an Arduino that has multiple variants. If you pick the wrong voltage, your device won't show up and you will need to follow the [![rescuing](https://sanjay900.github.io/guitar-configurator/tool/rescuing.html){: width="250" }](https://sanjay900.github.io/guitar-configurator/tool/rescuing.html) instructions to make it show up again!
```

## The finished product (When using a 3.3v Pro Micro and dupont wires)
[![Finished adaptor](../assets/images/adaptor-ps2.jpg){: width="250" }](../assets/images/adaptor-ps2.jpg)

## Steps
Refer to the following image for the pinout of a PS2 controller.

[![Finished adaptor](../assets/images/ps2-pinout.jpg){: width="250" }](../assets/images/ps2-pinout.jpg)

1. Connect Attention, Acknowledge to Acknowledge, Command to MOSI, Data to MISO and Clock to SCK. Note that for anything that isn't the 3.3v pro micro or pi pico, you will need to connect these pins using a level shifter.
   
   | Microcontroller | SCK | MOSI | MISO | Acknowledge | Attention |
   | --- | --- | --- | --- | --- | --- |
   | Pi Pico | GP6 | GP3 | GP4 | GP7 | GP10 |
   | Pro Micro, Leonardo, Micro | 15 | 16 | 14 | 7 | 10 |
   | Uno, Pro Mini | 13 | 11 | 12 | 2 | 10 |
   | Mega | 52 | 51 | 50 | 2 | 10 |
3. Connect a 1kΩ resistor between MISO and 3.3v.
4. Connect a 1kΩ resistor between Acknowledge and 3.3v.
5. Connect the 3.3v pin on the ps2 controller to the 3.3v pin on your Arduino. For arduinos that can output 3.3v natively, you can hook this straight up to 3.3v, but for a 5v only board like the pro micro 5v you will need to use a 3.3v voltage regulator to get a stable 3.3v voltage.
6. Connect the gnd pin on the wii breakout / extension cable to the gnd on your Arduino.
7. Start the Ardwiino Configuration Tool
   1. Note if you are on linux, you may need to follow the steps outlined in the ![Linux](linux.html) instructions
8. Plug in your Arduino
9.  Your Arduino should show up, like in the following image.

    [![Arduino uno in home screen](../assets/images/device-pick-uno.png){: width="250" }](../assets/images/device-pick-uno.png)
    * Using a Pico and don't see your device? Unplug the pico, hold the bootsel button, and plug it back in.
    
    [![PI Pico bootsel](../assets/images/pico-bootsel.png){: width="250" }](../assets/images/pico-bootsel.png)

10. Click Continue
11. You will be met with the following screen (Note that it will be slightly different for an Arduino Uno or Arduino Mega)

    [![Arduino programming](../assets/images/programming.png){: width="250" }](../assets/images/programming.png)

12. For the Micro, Leonardo and Pro Micro, please pick the type of device you have in the dropdown. Note that you need to get the voltage right, otherwise your device will not work and will need to be reset.
13. Click `Start programming` and wait for it to program
14. Once it finishes, Click `Start Configuring`
15. You will be taken to following the configuration screen.

    [![Graphical config](../assets/images/main-screen-graphical.png){: width="250" }](../assets/images/main-screen-graphical.png)

16. Click on `Change Device Settings`.
17. You will be shown the following dialog:

    [![Graphical config](../assets/images/change-dialog.png){: width="250" }](../assets/images/change-dialog.png)

18. Change `Controller Connectivity Type` to PS2.
    * You can also change the `Controller Output Type` here too if you would like to emulate a different type of controller, such as a drum set, a standard controller or even a controller for a different console, like a PS3 or a Switch.
    * Optionally, enable and configure Queue based inputs. This uncaps the scanning rate on the buttons of the controller (although this will still be limited by the internal PS2 circuitry), logging all the button changes to an internal list. The game then receives the next recorded input in the list instead of the immediate button state, which would usually be capped to the Controller Poll Rate. This emulates a higher polling rate even with low in-game FPS, which is extremely useful in games such as Clone Hero. Adjust the Dequeue Rate to be as close to, but still below your expected worst case in-game FPS for the system to work properly.
19. If you would like to emulate the Home button by hitting both Start and Select at the same time, then enable `Map Start + Select to Home`
20. Hit Close
21. Click `Write`
22. At this point, your controller should be working, test it using the Windows controller tester or a game controller tester of your choice.
