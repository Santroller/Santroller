---
sort: 6
---

# Setting up an Adaptor for a PS2 Controller

## Supplies

- A microcontroller

  - The Pi Pico is currently the only supported microcontroller. You will need to use an [older build](https://github.com/Santroller/Santroller/releases/tag/v10.1.188) if you wish to use any of the other microcontrollers listed below, and no support will be provided as this build is very old. This build is also missing quite a few features from the more recent builds, and in the future Santroller will only support the Pi Pico, or other boards based on it.
    {% include sections/microcontrollers.md %}

- For any 5v microcontrollers, you will need a 3.3v voltage regulator.
- PS2 Controller socket
- 2x 1kΩ Resistor
- Some Wire
- A Soldering Iron

Be careful that you don't ever provide 5v power to any pins on the PS2 Controller. While some people have done this successfully and just use their controllers with 5v, I can't recommend this approach as the controllers are really designed for 3.3v, so it may damage them slowly over time.

## Steps

1. Connect pins according to the below table.

   For beginners we provide a recommended pinout below for each microcontroller. These pinouts are the same pinouts that are used on the old Ardwiino firmware, so if you have an old device, this is the pinout it will be using.
   If these pins don't work for you, you can use one of the advanced pinouts below. Note that the Pi Pico has multiple channels, and SCK, MOSI and MISO need to be from the same channel.

   [![pinout](/assets/images/ps2.png)](/assets/images/ps2.png) [![Adapter pinout](/assets/images/ps2-pinout.png)](/assets/images/ps2-pinout.png)

   | Microcontroller                          | SCK / Clock / Blue | MOSI / Command / Orange | MISO / Data / Brown  | Acknowledge / Green    | Attention / Yellow |
   | ---------------------------------------- | ------------------ | ----------------------- | -------------------- | ---------------------- | ------------------ |
   | Pi Pico (Recommended)                    | GP6                | GP3                     | GP4                  | GP7                    | GP10               |
   | Pro Micro, Leonardo, Micro (Recommended) | 15                 | 16                      | 14                   | 7                      | 10                 |
   | Uno (Recommended)                        | 13                 | 11                      | 12                   | 2                      | 10                 |
   | Mega (Recommended)                       | 52                 | 51                      | 50                   | 2                      | 10                 |
   | Pi Pico (Advanced, Channel 0)            | GP2, GP6, GP18     | GP3, GP7, GP19          | GP0, GP4, GP16, GP20 | Any                    | Any                |
   | Pi Pico (Advanced, Channel 1)            | GP10, GP14, GP26   | GP11, GP15, GP27        | GP8, GP12, GP28      | Any                    | Any                |
   | Pro Micro, Leonardo, Micro (Advanced)    | 15                 | 16                      | 14                   | 0, 1, 2, 3 or 7        | Any                |
   | Uno (Advanced)                           | 13                 | 11                      | 12                   | 2 or 3                 | Any                |
   | Mega (Advanced)                          | 52                 | 51                      | 50                   | 2, 3, 18, 19, 20 or 21 | Any                |

2. Connect a 1kΩ resistor between MISO and 3.3v.
3. Connect a 1kΩ resistor between Acknowledge and 3.3v.
4. Connect the 3.3v pin on the ps2 controller to the 3.3v pin on your microcontroller. For microcontrollers that can output 3.3v natively, you can hook this straight up to 3.3v, but for a 5v only board like the Pro Micro 5v you will need to use a 3.3v voltage regulator to get a stable 3.3v voltage.
5. Connect the gnd pin on the PS2 controller to the gnd on your microcontroller.

## Programming

1.  Start Santroller with your microcontroller plugged in.
2.  Set the Input Type to `PS2 Adapter`
3.  Hit Configure
4.  Click on `Controller Settings`
5.  Set `Emulation Type` to `Controller` for controllers, or `Fortnite Festival` if you want to emulate a keyboard for `Fortnite Festival`
6.  Set the `Controller Type` based on the game you want to play.
7.  `Windows controller mode` can be set based on your preferences. Note that this only affects windows, a controller in XInput mode will use the correct mode on a console, and will automatically use HID mode on Linux and macOS.
    1. `XInput` - This works more natively on windows, and most games will automatically bind controls.
    2. `HID` - This uses HID on windows, which means games won't automatically bind controls, but HID is polled a bit more efficiently in games like Clone Hero.
8.  If you would like to adjust settings related to polling, click on `Controller Poll Settings`
9.  If you would like to use `Queue Based Inputs` you can turn that on here. This puts any buttons you press into a queue, and sends them out to the PC as fast as USB allows. This means that the controller will process your inputs at a faster poll rate than 1ms (the fastest rate USB allows) and then the PC will be sent your inputs at a 1ms rate.
10. Set the Poll Rate to your preferred setting. 0 sends data as fast as possible, any other number polls inputs at that speed.
11. Debounce can be adjusted here. Debounce is necessary as button inputs are noisy. When you hit a button, it will often bounce and send multiple presses, which some games may percieve as you hitting the button multiple times, which can result in dropped sustains. When you set debounce to a value, the signal ignores any release inputs for that time frame, so if you for example set it to 1ms, then the button input will be on for a minimum of 1ms, and only after that will the release be processed. This has the effect of stretching out the button press to at least 1ms, which ignores any bouncing in that timeframe.
12. Combined strum debounce shares the debounce timeframe between both strums (only available on guitars). This means that if you set the strum debounce to 1ms and strummed down, both strum up and down inputs are ignored over that 1ms timeframe. This helps avoid extra strum inputs when your strum switch rebounds after being released.
13. Click on `PS2 Controller Inputs`
14. Set the Acknowledge and Attention pins
15. For the Pi Pico, also specify your MOSI, MISO and SCK pins.
16. Hit Save.
17. You can now calibrate your inputs. If your controller is already plugged in it should be detected and show only those inputs, but you can use the `Wii Inputs to display below` dropdown to specify the controller you want to configure.
    1.  Joysticks
        1.  Hit the `Calibrate` button and then move the stick all the way left / up
        2.  Hit `Next` and then move the stick all the way right / down
        3.  Hit `Next` and then move stick a little bit away from the center. This sets the deadzone, and any inputs closer to the center from this position will be ignored, which helps with noisy sticks.
    2.  Whammy
        1.  Click on `Calibrate`.
        2.  Release the whammy bar and hit `Next`.
        3.  Push the whammy all the way in, and hit `Next`
        4.  Release the whammy again, and hit `Next`. If your whammy is noisy, you can push it in a tiny bit, and the zero position will be set to this location, which will make sure that the whammy is always considered released when it is released.
