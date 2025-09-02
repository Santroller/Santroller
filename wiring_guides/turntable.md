---
sort: 4
---

# Setting up a Turntable

## Wiring

### Supplies

- A microcontroller

  - The Pi Pico is currently the only supported microcontroller. You will need to use an [older build](https://github.com/Santroller/Santroller/releases/tag/v10.1.188) if you wish to use any of the other microcontrollers listed below, and no support will be provided as this build is very old. This build is also missing quite a few features from the more recent builds, and in the future Santroller will only support the Pi Pico, or other boards based on it.
    {% include sections/microcontrollers.md %}

- Some Wire
- Soldering Iron
- Multimeter (it will be used mainly in continuity mode, where it beeps when the two contacts are shorted together)
- Wire Strippers
- Wire Cutters
- Heatshrink

### Wiring Steps

{% include sections/getting_started.md %}

To keep the relevant information in this guide easy to find, information is sorted by function.

<details>
    <summary>Effects Knob</summary>

1. Wire one of the outer pins to V<sub>CC</sub>
2. Wire the other outer pin to GND
3. Wire the middle pin to an analog pin on your microcontroller.
</details>

<details>
    <summary>Crossfader</summary>

1. Follow the pins going to the Crossfader. There should be three pins.
2. Wire one of the outer pins to V<sub>CC</sub>
3. Wire the other outer pin to GND
4. Wire the middle pin to an analog pin on your microcontroller.
</details>

<details>
   <summary>Euphoria Button and LED</summary>

1.  Connect V to V<sub>CC</sub> on your microcontroller.
2.  Connect V0 to ground on your microcontroller.
3.  Connect AT to one of the following pins

    | Microcontroller            | Analog Output Pins     |
    | -------------------------- | ---------------------- |
    | Pi Pico (Recommended)      | Any                    |
    | Pro Micro, Leonardo, Micro | 3, 5, 6, 9, 10, 11, 13 |
    | Uno                        | 3, 5, 6, 9, 10         |
    | Mega                       | 2 - 13, 44 - 46        |

</details>

<details>
    <summary>Face buttons</summary>

1. Remove the face button PCB from the turntable
2. There is a smaller PCB attached to the main button PCB with headers, which contains the main micro controller. Disconnect this in some way, either by cutting it away, desoldering it or cutting traces going to the microcontroller on it. Be careful though, as the traces on these PCBs are very cheap and can break very very easily.
3. Follow the traces from each pad. You will find that there will be common traces going to multiple pads, which you can connect to ground. Connect the other trace going to each pad to a digital pin on your microcontroller. In effect, you should end up with one side of every pad going to ground, and the other side going to a digital pin. Some tables may even have the pins labeled for you, which can make things easier.
</details>

<details>
    <summary>Platter (Standard)</summary>

1.  The platters connect to the same PCB that the crossfader is on. There will be some labelled pads, V, C, D and G.
2.  Hook up V to V<sub>CC</sub> and G to GND. For safety I would recommend hooking V to 3.3v on a 5v microcontroller, as most I2C devices I have come across don't like being powered by 5v.
3.  Hook up the C to SCL and D to SDA. The Pi Pico lets you pick from various pins for the SDA and SCL pins. We provide recommended pins below. If you need to use other pins, the options are provided below but the SDA and SCL pins must be from the same channel.

| Microcontroller               | SDA (D)                          | SCL (C)                          |
| ----------------------------- | -------------------------------- | -------------------------------- |
| Pi Pico (Recommended)         | GP18                             | GP19                             |
| Pro Micro, Leonardo, Micro    | 2                                | 3                                |
| Uno                           | A4                               | A5                               |
| Mega                          | 20                               | 21                               |
| Pi Pico (Advanced, Channel 0) | GP0, GP4, GP8, GP12, GP16, GP20  | GP1, GP5, GP9, GP13, GP17, GP21  |
| Pi Pico (Advanced, Channel 1) | GP2, GP6, GP10, GP14, GP18, GP26 | GP3, GP7, GP11, GP15, GP19, GP27 |

</details>

<details>
    <summary>Platter (Peripheral)</summary>

If you would like your frets to contain LEDs, or want your frets to poll at a different rate to the rest of the turntable, you can use the [Peripheral feature](https://santroller.tangentmc.net/wiring_guides/peripheral.html). Note that you can currently only have a single peripheral, so you will not be able to use two peripherals to have multiple turntables connected at once.

1.  Open up the platter on your turntable
2.  Desolder the CLK and DATA from your platter, and solder them to the peripheral pins that you have chosen.
3.  Desolder the 3V3 (Red) line from the platter and solder it to VBUS on your peripheral. Do the same on the main pico, you should end up with VBUS connected to VBUS via the slip ring and platter connector.
4.  Connect CLK and DATA. The Pi Pico lets you pick from various pins for the SDA and SCL pins. We provide recommended pins below. If you need to use other pins, the options are provided below but the SDA and SCL pins must be from the same channel.

    | Microcontroller               | SDA (DATA, Yellow)               | SCL (CLK, Green)                 |
    | ----------------------------- | -------------------------------- | -------------------------------- |
    | Pi Pico (Recommended)         | GP18                             | GP19                             |
    | Pi Pico (Advanced, Channel 0) | GP0, GP4, GP8, GP12, GP16, GP20  | GP1, GP5, GP9, GP13, GP17, GP21  |
    | Pi Pico (Advanced, Channel 1) | GP2, GP6, GP10, GP14, GP18, GP26 | GP3, GP7, GP11, GP15, GP19, GP27 |

5.  Connect 3V3 out to the 3V3 pin on your platter
6.  Disconnect the buttons from the platter, and connect them to digital pins your peripheral Pico.
7.  Connect ground on the peripheral to ground (Blue) on the platter. This should also still be connected to ground on the main Pico.
8.  There should be a common ground going to the frets, and then seperate signal wires. Disconnect the signal wires for the frets from the platter, and then connect them to digital pins on the peripheral pico.

Note: The pinouts for the various pins going through the slip ring are as follows:

- YELLOW: DATA (D)
- BLUE: GND (G)
- GREEN: CLK (C)
- RED: 3V3 (V)
- WHITE: SENSE PIN (ID)

</details>

## Programming

### Peripheral

If you intend to use the peripheral features, it is recommended to program the peripheral pico first. Follow the below instructions to do that, or skip these if you are not using this feature.

1.  Plug the peripheral pico into your computer. Make sure the main pico is not plugged in at this stage.
2.  Open Santroller
3.  Set the Input Type to `Peripheral Device`
4.  Pick the SCL and SDA pins
5.  Hit configure
6.  Now you can unplug the peripheral pico from your comptuter, and follow the rest of the instructions

### Main

1.  Start Santroller with your microcontroller plugged in.
2.  Set the Input Type to Directly Wired
3.  Hit Configure
4.  Click on `Controller Settings`
5.  Set `Emulation Type` to `Controller`
6.  Set the `Controller Type` to `DJ Hero Turntable`.
7.  `Windows controller mode` can be set based on your preferences. Note that this only affects windows, a controller in XInput mode will use the correct mode on a console, and will automatically use HID mode on Linux and macOS.
    1. `XInput` - This works more natively on windows, and most games will automatically bind controls.
    2. `HID` - This uses HID on windows, which means games won't automatically bind controls, but HID is polled a bit more efficiently in games like Clone Hero.
8.  If you would like to adjust settings related to polling, click on `Controller Poll Settings`
9.  If you would like to use `Queue Based Inputs` you can turn that on here. This puts any buttons you press into a queue, and sends them out to the PC as fast as USB allows. This means that the controller will process your inputs at a faster poll rate than 1ms (the fastest rate USB allows) and then the PC will be sent your inputs at a 1ms rate.
10. Set the Poll Rate to your preferred setting. 0 sends data as fast as possible, any other number polls inputs at that speed.
11. Debounce can be adjusted here. Debounce is necessary as button inputs are noisy. When you hit a button, it will often bounce and send multiple presses, which some games may percieve as you hitting the button multiple times, which can result in dropped sustains. When you set debounce to a value, the signal ignores any release inputs for that time frame, so if you for example set it to 1ms, then the button input will be on for a minimum of 1ms, and only after that will the release be processed. This has the effect of stretching out the button press to at least 1ms, which ignores any bouncing in that timeframe.
12. Now you can start setting up your inputs. To keep this information relevant, it is grouped by function.
    <details>
      <summary>The Platter</summary>

    1. Click on `Dj Turntable Inputs`.
    2. Set the SCL and SDA pins that you have wired the platter to.
    3. Leave the turntable poll rate at 10ms. If you poll it any quicker than this, the platter velocity will be less accurate due to how it is polled.

    </details>
    <details>
      <summary>Buttons or Dpad</summary>

    1. Click on the button you want to configure, and make sure the `Input Type` is set to `Digital Pin Input`.
    2. Make sure `Pin Mode` is set to `Pull Up`.
    2. Click on the `Find Pin` button, and then press the button on the guitar. If you have wired everything correctly, the tool should detect the pin and the icon for that button should now light up whenever the button is pressed.

    </details>

    <details>
      <summary>Joystick</summary>

    1. Click on D-pad Left, and set the `Input Type` to `Analog Pin Input`.
    2. Set `Type` to `Joystick Negative`
    3. Click on find pin and move the joystick left or right
    4. Adjust the threshold so that the D-pad Left icon lights up when you have pushed the Joystick far enough to the left. This means you can adjust how sensitive you want your joystick to be.
    5. You can do the same for D-pad right, however, set the `Type` to `Joystick Positive` instead.
    6. If you wish to also map joystick up and down, click `Add Setting` and add another Strup Up and Strum Down input. Then you can follow the above instructions again, only using negative for up and positive for down, and when detecting the pin, move the joystick up and down instead.

    </details>

    <details>
      <summary>Crossfader</summary>

    1. Click on the crossfader, and make sure the `Input Type` is set to `Analog Pin Input`.
    2. Click on the `Find Pin` button, and then press on the whammy. If you have wired everything correctly, the tool should detect the pin and the `Original Value` value should change when you push on the whammy.
    3. Click on `Calibrate`.
    4. Move the crossfader all the way left and hit `Next`.
    5. Move the crossfader all the way right, and hit `Next`
    6. Move the crossfader to the center, and hit `Next`.
    </details>

    <details>
      <summary>Effects Knob</summary>

    1. Click on the Effects Knob, and make sure the `Input Type` is set to `Analog Pin Input`.
    2. Click on the `Find Pin` button, and then press on the whammy. If you have wired everything correctly, the tool should detect the pin and the `Original Value` value should change when you rotate the effects knob.
    </details>

    <details>
      <summary>Peripheral</summary>

    1. Click on `Peripheral Settings`
    2. Enable the Peripheral
    3. Set the SDA and SCL pins on the main PCB that are being connected to the peripheral.
    4. Hit save
    5. Click on `Dj Turntable Inputs`
    6. Disable all the frets
    7. Click on `Add Setting`
    8. Add each fret for the turntable you are configuring
    9. For each fret: 
       1.  Click on the Fret 
       2.  Set the Input Type to `Digital Pin Input (Peripheral)` 
       3.  Make sure `Pin Mode` is set to `Pull Up`.
       4.  Click on `Find Pin` 
       5.  Press the fret in question. The tool should detect this and the fret should light up in the tool when pressed.
    </details>

    <details>
      <summary>USB Host (Pi Pico Only)</summary>

    10. Click on Add setting
    11. Find and add `USB Host inputs`
    12. Bind D+
    13. Hit Save
    14. If you plug in a supported controller, the tool should detect it and tell you what it is.
    15. If you have a modded xbox and are using `usbdsecpatch`, you can disable `Authentication for Xbox 360`.

    </details>

13. If you do not want to hook an input up, hit the `Remove` button to the right on that input.
14. Once everything is configured correctly, the `Save Settings` button should be clickable and you can hit that button to write your config to the guitar. It is now ready.
