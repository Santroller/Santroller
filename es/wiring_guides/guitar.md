---
sort: 2
---

# Setting up a Guitar ES

[![Finished guitar](/assets/images/direct.jpg)](/assets/images/direct.jpg)

## Wiring

### Supplies

- A microcontroller

  - The Pi Pico is recommended, but click below to see information about other microcontrollers.
    {% include sections/microcontrollers.md %}

- Some Wire
- Soldering Iron
- Multimeter (it will be used mainly in continuity mode, where it beeps when the two contacts are shorted together)
- Wire Strippers
- Wire Cutters
- Heatshrink

- Supplies for specific features.
  - Tilt
    - Any of the following options are supported.
      - A Digital Tilt Switch (somtimes called a mercury or ball tilt switch)
        - It is recommended using two tilt sensors in series, as this can help with accidental activations.
      - An ADXL345, MPU-6050 or LIS3DH Accelerometer.
        - Gives a proper analog value of the devices acceleration.
          - Tilt is detected by observing acceleration due to gravity.
      - An analog accelerometer, such as the ADXL335, or the accelerometer on certain GH guitars.
        - This gives a proper analog value of the devices acceleration
          - Tilt is detected by observing acceleration due to gravity
  - Guitar Hero World Tour Guitar
    - MPR121
      - Required for reusing the slider bar from a Guitar Hero World Tour Guitar.
  - Guitar Hero 5 Guitar or Crazy Guitar
    - Standard Mode, worst latency.
      - No extra parts required.
    - MPR121 Mode, great latency
      - Requires a MPR121, replaces the microcontroller on the slider bar and handles both slider and frets.
    - Peripheral Pico Mode, great latency.
      - Requires a second Pi Pico, places the second Pi Pico in the neck and uses that to poll the frets, while leaving the original slider bar pcb to poll the slider bar, sharing power and data between both devices.
    - Hardwire frets, great latency.
      - No extra parts required, but sacrifices the detachable neck.
  - USB Host
    - Pi Pico
    - One of the following
      - A USB female breakout.
      - A USB extension cable.
      - A controller with a cable already attached that you are willing to cut.
        - Needs to be a wired controller, wireless only controllers won't work, nor will the play and charge kit.

### Wiring Steps

{% include sections/getting_started.md %}

For every function of the guitar, you will follow these same 3 basic steps. More details for specific functions will be given below.

1. Find out what each pin correlates to on the exsisting components in the guitar. (AKA work out the pinout) In some guitars, all of the pins are labeled on the main board or other various pcbs, sometimes on the bottom of the main board.
2. Cut the wires away from the main board as close to the PCB as you can, giving yourself the most amount of wire that you can get. You may find however that the wire isn't long enough, in this case you can either join some more wire onto the end of the existing wires or you can desolder the cables and replace them.
3. Connect the wires to appropriate pins on the microcontroller.

Once you have successfully modded one guitar (or even during your first) you will find it easiest to disconnect everything then begin soldering as it gives you more room to work. To keep the relevant information in this guide easy to find, information is sorted by function.

<details>
    <summary>Start and Select</summary>

[![360startselect](/assets/images/360startselect.jpg)](/assets/images/360startselect.jpg)

[![wiilpstartselect](/assets/images/wiiLPstartselect.jpg)](/assets/images/wiiLPstartselect.jpg)

1. If it is not labeled on the motherboard, you need to figure out which pin (or pins) is GND. If you remove the membrane, you will be able to see which pin connects to both start and select by following the traces - that pin is GND. (if there are multiple "grounds" they may be labeled as col, or column) Marking GND with a sharpie or using a differently colored wire is reccomended. If you are still unsure, the multimeter can help you confirm if you have the wires correct, as it will beep when you click a button and have the correct wires. There are examples of the 360 WT and WiiLP above.
2. Connect the common ground to a GND on the microcontroller. If there are two ground wires, you can twist them together and put them in one ground pin on the microcontroller. This will sometimes be necessary when using a microcontroller that has a smaller number of pins.
3. Connect each button to an unused digital pin on the microcontroller.

```note
To wire an xplorer start/select board you will have to solder directly to the contacts and cut the traces that connect to each contact. It is not recommended you try to mod an xplorer as your first project because this can be difficult.
Note that with the Pi Pico, you can instead just hook these inputs up over USB, and avoid soldering them.
```

</details>

<details>
    <summary>Whammy</summary>

1. With most whammy bars, there are 3 pins. (if there are 4, you do not need the fourth. 2 pins is discussed below) The middle pin is the data pin, one outer pin is V<sub>CC</sub> while the other pin is GND (it doesn't matter which one, you can flip it when programming later if needed). Whammy needs to go to an analogue pin.
2. Connect V<sub>CC</sub> and GND to the microcontroller. (the two outside wires)
   - If your whammy isn't responding properly in game, you may need to swap V<sub>CC</sub> and GND around.
3. Connect the data (middle) pin to an analogue pin on the microcontroller. These are labeled with an A on the microcontroller.

If you are working on a controller with only "2" pins as shown below, you will need to desolder the bridged pins and run 3 wires yourself as noted above for the whammy to work best with the configurator.

[![2pinwhammy](/assets/images/2wirewhammy.jpg)](/assets/images/2wirewhammy.jpg)

</details>

<details>
    <summary>Pickup Selector</summary>

1. With most pickup selectors's there are 3 pins. (if there are 4, you do not need the fourth. 2 pins is discussed below) The middle pin is the data pin, one outer pin is V<sub>CC</sub> while the other pin is GND (it doesn't matter which one, you can flip it when programming later if needed). Whammy needs to go to an analogue pin.
2. Connect V<sub>CC</sub> and GND to the microcontroller. (the two outside wires)
   - If your whammy is not responding correctly in game, you may need to swap V<sub>CC</sub> and GND around.
3. Connect the data (middle) pin to an analogue pin on the microcontroller. These are labeled with an A on the microcontroller.

If you are working on a controller with only "2" pins as the RB Precision Bass, you will need to desolder the bridged pins and run 3 wires yourself as noted above for the pickup selector to work best with the configurator. For the Precision Bass this is easiest done by desoldering the PCB attached to the potentiometer, and then running your own wires directly to the potentiometer.

</details>

<details>
    <summary>Digital Tilt</summary>

1. Connect one pin of the first tilt sensor to ground
2. Connect the other pin of the first tilt sensor to a pin on the second tilt sensor
3. Connect the other pin of the second tilt sensor to a digital pin on your microcontroller.
4. Affix the sensors to your guitar. You will need to play around with their position to get them to activate at the exact point you want tilt activating.
</details>

<details>
    <summary>Analog Tilt</summary>

1. Connect GND to GND
2. Connect V<sub>CC</sub> to V<sub>CC</sub>
3. Connect the signal pin to an analog pin on your microcontroller
</details>

<details>
    <summary>ADXL345 Or MPU 6050 or LIS3DH</summary>

1. Connect GND to GND
2. Connect V<sub>CC</sub> to V<sub>CC</sub> (note that this is a 3.3v device, so for 5V microcontrollers make sure your breakout has a voltage regulator onboard or you are using your own regulator)
3. Hook up SDA and SCL to the microcontroller

   | Microcontroller               | SDA                              | SCL                              |
   | ----------------------------- | -------------------------------- | -------------------------------- |
   | Pi Pico (Recommended)         | GP18                             | GP19                             |
   | Pro Micro, Leonardo, Micro    | 2                                | 3                                |
   | Uno                           | A4                               | A5                               |
   | Mega                          | 20                               | 21                               |
   | Pi Pico (Advanced, Channel 0) | GP0, GP4, GP8, GP12, GP16, GP20  | GP1, GP5, GP9, GP13, GP17, GP21  |
   | Pi Pico (Advanced, Channel 1) | GP2, GP6, GP10, GP14, GP18, GP26 | GP3, GP7, GP11, GP15, GP19, GP27 |

   </details>

<details>
    <summary>Joystick (or DPad)</summary>

For d-pads that are integrated with the main board it is advised you skip wiring the dpad as you have to solder directly to the contacts and run wires accross the board. You will either be able to use your keyboard for these buttons, or they really won't really be needed as the games were designed to be controlled with the guitar alone. Below is an example of what this can look like, and why it is advisable to skip.

[![curseddpad](/assets/images/curseddpad.jpg)](/assets/images/curseddpad.jpg)

For guitars with a DPad that is seperate, it will be much easier to wire as you can follow the traces and wire it to the pins like you would for start/select.

[![wtdpad](/assets/images/wtdpad.jpg)](/assets/images/wtdpad.jpg)

1.  Find ground. There will be a single common ground and a pin for each direction or multiple "grounds" depending on the model. (some may once again be labeled as col or column) Just like for start and select, you will need to follow the traces to figure out which pin is GND. Mark GND, then connect it to a GND pin on the microcontroller. If there are more than one ground wires, you can twist them together and combine them again.
2.  Up and down on the dpad MUST be connected to the same pins you will be using for strum. You will want to twist those wires together and solder them to the same pin. You may want to wait until you are working on the strum to connect these pins.
3.  Home, left, and right can be connected to any unused digital pin on the microcontroller.

For guitars with a joystick, there will be four pins, one is V<sub>CC</sub>, one is GND, one is the x axis and one is the y axis. You can work out which is which by tracing the traces, however on some guitars the traces are labelled for you. The joystick needs to go to an analogue pin (one of the A pins)

</details>

<details>
    <summary>Frets</summary>

1. For the frets, if it is not labeled it is easiest to open up the neck and follow the traces between the fret contacts. The ground wire traces will connect to all of the fret contacts, whereas a fret trace will lead to a single fret contact. If using the multimeter, test between the fret wire and the ground wire, and the multimeter should beep when the fret is pressed.
2. Connect the common grounds to a ground pin on the microcontroller.
3. Connect each fret to its own unused digital pin.
</details>

<details>
    <summary>RB Frets</summary>
With some RB guitars, the solo frets are combined with the standard frets. These guitars have 7 wires, as they then just have a single `solo` input that gets sent if any solo frets are pressed. This means that these guitars by default won't be able to tell if you are pressing both normal and solo frets at the same time, but that isn't something you should ever really need.

For these guitars, you have multiple options on how you can wire up the frets.

#### Run new wires
Since most of these guitars are hardwired, you can opt to just run new wires yourself from the standard fret PCB, which would then give you seperate fret inputs. In this case, you do not need to connect up the solo input, as you are bypassing it.

1. For the frets, if it is not labeled it is easiest to open up the neck and follow the traces between the fret contacts. The ground wire traces will connect to all of the fret contacts, whereas a fret trace will lead to a single fret contact. If using the multimeter, test between the fret wire and the ground wire, and the multimeter should beep when the fret is pressed.
2. Connect the common grounds to a ground pin on the microcontroller.
3. Connect each fret to its own unused digital pin.

#### Wire up the solo input
You can also just wire up the solo input to a pin on your microcontroller, and then when programming you can follow some different steps to make the solo frets function.

1. For the frets, if it is not labeled it is easiest to open up the neck and follow the traces between the fret contacts. The ground wire traces will connect to all of the fret contacts, whereas a fret trace will lead to a single fret contact. If using the multimeter, test between the fret wire and the ground wire, and the multimeter should beep when the fret is pressed.
2. Connect the common grounds to a ground pin on the microcontroller.
3. Connect each fret to its own unused digital pin.
4. Connect the solo input to its own unused digital pin.

</details>

<details>
    <summary>Strum</summary>

The Strum switches are similar to the start and select buttons, they will be three wires on some guitars. For these situations it is easy enough to connect to the microcontroller.

1. Connect Strum
2. Connect the common ground to a GND on the microcontroller.
3. Connect each strum switch to seperate unused pins on the microcontroller.

On others, the Strum switches are a part of the main PCB, and you will need to solder directly to the strum switches, which should poke out the back of the main PCB. For example, on a wiitar, you will see the following:

[![Wii Strum PCB](/assets/images/wii-strum.jpg)](/assets/images/wii-strum.jpg)

In this case, there are two grounds that will be shorted together, so with the multimeter, you should be able to work out which pins are ground, by testing a pin from each switch, and working out which ones are shorted together by it beeping.

Note that you can also choose to replace the original PCB with a 3D printed strum switch holder. If you want to go that route, there are some designs around for various guitars. The image at the beginning of this guide shows how this would look.

When the strums are part of the main board you will need to cut the traces or you will have phantom inputs as your signal will still be traveling through the motherboard. (this is when your strum switches constantly input and you likely cannot autobind inputs in the configurator) You will need to take a knife and cut any traces that connect to the strum switches. In the picture below, the person did not cut many traces as they knew which ones were causing phantom inputs. Cutting extra traces is not going to affect your arduino guitar, as none of the traces are used except the one that connects the two grounds of the switches together. Even if you accidentally cut that trace, you will be able to connect the grounds again with a little extra wire.

[![Trace Cuts on PCB](/assets/images/trace%20cuts.jpg)](/assets/images/trace%20cuts.jpg)

</details>

<details>
    <summary>USB Host (Pi Pico Only)</summary>
If you want to use your controller on an unmodifed Xbox 360 or Xbox One or Xbox Series, you can wire a USB port to the Pi Pico. You can also use this feature if your  guitar is USB based (like the xplorer) and you would rather pass some inputs through instead of wiring them manually, such as the dpad or start and select on the xplorer.

[![usb](/assets/images/usb.png)](/assets/images/usb.png)

1. If you are using a USB extension cable, cut it in half and expose the four cables.
2. Hook up the V+ / VBUS (Red) to the VBUS pin on your Pi Pico
3. Hook up the V- / GND (Black) to ground on your Pi Pico
4. Hook up D+ (Green) to a unused digital pin.
5. Hook up D- (White) to the digital pin directly after D+. For example, you can hook up D+ to GP2 and D- to GP3.

</details>

<details>
    <summary>Guitar Hero World Tour neck</summary>

The world tour slider bar originally used a single wire to connect between the bar and the main PCB. This caused a lot of problems, as the format of data being sent over this wire is not optimal for speed, and it limits the combinations of frets we can read from the slider bar. To combat this, we bypass the chip generating this data, and opt to use a dedicated touch sensor chip, the MPR121, to handle reading from the slider bar and sending us data in a format that we can easily use.

1. Cut the traces indicated with red lines in the following image on your Slider bar PCB. You can also opt to just desolder U2 if you prefer.

   [![World tour slider traces](/assets/images/wt_traces.png)](/assets/images/wt_traces.png)

2. Solder each of the circled vias on the WT Slider bar PCB to its own channel on the MPR121. It is recommended to use channels 0-4 for the slider bar, as the MPR121 requires assigning touch pads first before other pins.

   [![World tour slider pins](/assets/images/wt_vias.png)](/assets/images/wt_vias.png)

3. For the frets, follow the traces between the fret contacts. The ground wire traces will connect to all of the fret contacts, whereas a fret trace will lead to a single fret contact. At the end of this guide, there are some images for known neck pinouts. If using the multimeter, test between the fret wire and the ground wire, and the multimeter should beep when the fret is pressed.
4. Connect the common grounds to a ground pin on the MPR121.
5. Connect each fret to its own unused channel on the MPR121.
6. Hook up V<sub>CC</sub> (marked as 3.3V) and GND (marked as GND) to the microcontroller. Note that this means you need to use a 3.3V power pin on your microconroller, so if your microcontroller only has 5V power pins you will need a voltage regulator.
2. Hook up SDA and SCL to the microcontroller

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
    <summary>GH5 guitar neck (Standard)</summary>

1. Hook up V<sub>CC</sub> (marked as V or V<sub>CC</sub>) and GND (marked as GND or G) to the microcontroller
2. Hook up SDA (Marked as D) and SCL (marked as C) to the microcontroller

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
    <summary>Crazy guitar neck (Standard)</summary>

[![crazy guitar](/assets/images/crazy-guitar.png)](/assets/images/crazy-guitar.png)

1. Hook up V<sub>CC</sub>, GND, SCL and SDA pins to your microcontroller. Note that this is one of the few I2C devices that works perfectly fine on 5v.
2. Hook up SDA (Marked as D) and SCL (marked as C) to the microcontroller

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
    <summary>GH5 / Crazy guitar frets (Direct, lower latency)</summary>

1. If you wish to bypass the neck connector for your frets, there are three choices, you can opt to wire the frets directly from the fret pcb to the pico, you can use an MPR121 and avoid needing to run more wires, or you can use peripheral mode and avoid needing to run more wires. The MPR121 replaces the entire slider bar PCB and handles both the slider pads and the frets, while the peripheral only handles the frets, and requires using the original slider bar PCB for keeping slider functionality.
2. For direct mode, run a wire from the fret pin to a pin on the pico, and make sure there is a wire from the ground pin on the fret pcb to the picos ground pin.
</details>

<details>
    <summary>GH5 / Crazy guitar frets (MPR121, Low Latency Frets with 4 pins)</summary>

1. In some cases, you may want to poll frets directly, but you still wish to use a neck connector that does not have enough pins to connect the frets. You can get around this by using an MPR121. 

2. Cut the traces going from the main chip to the pads on the slider PCB.

3. Solder each via on the Slider bar PCB to its own channel on the MPR121. It is recommended to use channels 0-4 for the slider bar, as the MPR121 requires assigning touch pads first before other pins.

4. For the frets, follow the traces between the fret contacts. The ground wire traces will connect to all of the fret contacts, whereas a fret trace will lead to a single fret contact. At the end of this guide, there are some images for known neck pinouts. If using the multimeter, test between the fret wire and the ground wire, and the multimeter should beep when the fret is pressed.
5. Connect the common grounds to a ground pin on the MPR121.
6. Connect each fret to its own unused channel on the MPR121.
7. Hook up V<sub>CC</sub> (marked as 3.3V) and GND (marked as GND) to the microcontroller. Note that this means you need to use a 3.3V power pin on your microconroller, so if your microcontroller only has 5V power pins you will need a voltage regulator.
8. Hook up SDA and SCL to the microcontroller

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
    <summary>GH5 / Crazy guitar frets (Peripheral, Low Latency Frets with 4 pins)</summary>

1. In some cases, you may want to poll frets directly, but you still wish to use a neck connector that does not have enough pins to connect the frets. You can get around this by putting a second Pi Pico in the neck.
2. Choose some SDA and SCL pins on each Pico, and connect them by these pins.

   | Microcontroller               | SDA                              | SCL                              |
   | ----------------------------- | -------------------------------- | -------------------------------- |
   | Pi Pico (Recommended)         | GP18                             | GP19                             |
   | Pi Pico (Advanced, Channel 0) | GP0, GP4, GP8, GP12, GP16, GP20  | GP1, GP5, GP9, GP13, GP17, GP21  |
   | Pi Pico (Advanced, Channel 1) | GP2, GP6, GP10, GP14, GP18, GP26 | GP3, GP7, GP11, GP15, GP19, GP27 |

3. Connect the Slider bar to the same SDA and SCL pins
4. Connect VBUS and GND between the two Pi Picos.
5. Connect the Slider bar to 3.3v and GND on the peripheral Pico.
6. For the frets, if it is not labeled it is easiest to follow the traces between the fret contacts. The ground wire traces will connect to all of the fret contacts, whereas a fret trace will lead to a single fret contact. At the end of this guide, there are some images for known neck pinouts. If using the multimeter, test between the fret wire and the ground wire, and the multimeter should beep when the fret is pressed.
7. Connect the common grounds to a ground pin on the peripheral Pico.
8. Connect each fret to its own unused digital pin on the peripheral Pico.

   </details>

## Programming

### Peripheral

If you intend to use the peripheral features, it is recommended to program the peripheral pico first. Follow the below instructions to do that, or skip these if you are not using this feature.

1.  Plug the peripheral pico into your computer. Make sure the main pico is not plugged in at this stage.
2.  Open `Santroller`
3.  Set the Input Type to `Peripheral Device`
4.  Pick the SCL and SDA pins.
5.  Click `Configure`
6.  Now you can unplug the peripheral pico from your comptuter, and follow the rest of the instructions.

### Main

1.  Start Santroller with your microcontroller plugged in.
2.  Set the Input Type to Directly Wired
3.  Click `Configure`
4.  Click on `Controller Settings`
5.  Set `Emulation Type` to `Controller`
6.  Set the `Controller Type` based on the game you want to play.
7.  `Windows Controller Mode` can be set based on your preferences. Note that this is only for Windows, no other operating systems or consoles will be affected by this option.
    1. `XInput` - This works more natively on windows, and most games will automatically bind controls.
    2. `HID` - This uses HID on windows, which means games won't automatically bind controls, but HID is polled a bit more efficiently in games like Clone Hero.
8.  If you would like to adjust settings related to polling, click on `Controller Poll Settings`
9.  If you would like to use `Queue Based Inputs` you can turn that on here. This puts any buttons you press into a queue, and sends them out to the PC as fast as USB allows. This means that the controller will process your inputs at a faster poll rate than 1ms (the fastest rate USB allows) and then the PC will be sent your inputs at a 1ms rate.
10. Set the Poll Rate to your preferred setting. 0 sends data as fast as possible, any other number polls inputs at that speed.
11. Debounce can be adjusted here. Debounce is necessary as button inputs are noisy. When you click a button, it will often bounce and send multiple presses, which some games may percieve as you clicking the button multiple times, which can result in dropped sustains. When you set debounce to a value, the signal ignores any release inputs for that time frame, so if you for example set it to 1ms, then the button input will be on for a minimum of 1ms, and only after that will the release be processed. This has the effect of stretching out the button press to at least 1ms, which ignores any bouncing in that timeframe.
12. Combined strum debounce shares the debounce timeframe between both strums. This means that if you set the strum debounce to 1ms and strummed down, both strum up and down inputs are ignored over that 1ms timeframe. This helps avoid extra strum inputs when your strum switch rebounds after being released.
13. Now you can start setting up your inputs. To keep this information relevant, it is grouped by function.
  <details>
    <summary>Frets, strum and other buttons</summary>

  1. Click on the button you want to configure, and make sure the `Input Type` is set to `Digital Pin Input`.
  2. Make sure `Pin Mode` is set to `Pull Up`.
  2. Click on the `Find Pin` button, and then press the button on the guitar. If you have wired everything correctly, the tool should detect the pin and the icon for that button should now light up whenever the button is pressed.

  </details>

  <details>
    <summary>RB Solo Frets (Combined)</summary>
  
  In this example we will use the green solo fret, but you can substitute the fret for the other colours to set the rest of the frets up.
  1. Click on the green solo fret.
  2. Set the `Input Type` to `Shortcut`
  3. Set the next `Input Type` to `Digital Pin Input`
  2. Make sure `Pin Mode` is set to `Pull Up`.
  3. Click on the first `Find Pin` button, and then press the normal green fret button.
  4. Hold the green fret and then press the second `Find Pin` button. While holding the green fret, press the solo green fret. This should then detect the solo input, and you can release both buttons. At this point, the icon for the green solo fret should light up when it is pressed.
  </details>

  <details>
    <summary>Start + Select to Home</summary>

  1. Click on the home / Xbox / PS button.
  2. Set the `Input Type` to `Shortcut`
  3. Set the next `Input Type` to `Digital Pin Input`
  4. Make sure `Pin Mode` is set to `Pull Up`.
  5. Click on the first `Find Pin` button, and then press the start button on the guitar. If you have wired everything correctly, the tool should detect the pin.
  6. Click on the second `Find Pin` button, and then press the select button on the guitar. If you have wired everything correctly, the tool should detect the pin and the icon for the home button should now light up whenever both start and select is pressed.

  </details>

  <details>
    <summary>Joystick</summary>

  1. Click on D-pad Left, and set the `Input Type` to `Analog Pin Input`.
  2. Set `Type` to `Joystick Negative`
  3. Click on `Find Pin` and move the joystick left or right
  4. Adjust the threshold so that the D-pad Left icon lights up when you have pushed the Joystick far enough to the left. This means you can adjust how sensitive you want your joystick to be.
  5. You can do the same for D-pad right, however, set the `Type` to `Joystick Positive` instead.
  6. If you wish to also map joystick up and down, click `Add Setting` and add another Strum Up and Strum Down input. Then you can follow the above instructions again, only using negative for up and positive for down, and when detecting the pin, move the joystick up and down instead.

  </details>

  <details>
    <summary>Whammy</summary>

  1. Click on the whammy, and make sure the `Input Type` is set to `Analog Pin Input`.
  2. Click on the `Find Pin` button, and then press on the whammy. If you have wired everything correctly, the tool should detect the pin and the `Original Value` value should change when you push on the whammy.
  3. Click on `Calibrate`.
  4. Release the whammy bar and click `Next`.
  5. Push the whammy all the way in, and click `Next`
  6. Release the whammy again, and click `Next`. If your whammy is noisy, you can push it in a tiny bit, and the zero position will be set to this location, which will make sure that the whammy is always considered released when it is released.
  </details>

  <details>
    <summary>Pickup Selector</summary>

  1. Click on the pickup selector, and make sure the `Input Type` is set to `Analog Pin Input`.
  2. Click on the `Find Pin` button, and then move the pickup selector. If you have wired everything correctly, the tool should detect the pin and the `Original Value` value should change when you move the pickup selector.
  3. If the `Original Value` of the pickup selector decreases as you increase the notch position on your guitar, turn on `Invert`.
  4. Set the pickup selector to the second position
  5. Drag the slider for `Notch 2 Value` so that the current notch is `Notch 2`.
  6. Set the pickup selector to the third position and then move the slider for `Notch 3` as detailed above.
  7. Repeat this for the other notches. After this, the current notch should reflect the status of the pickup selector.
  </details>

  <details>
    <summary>Digital Tilt</summary>

  1. Click on Tilt, and make sure the `Input Type` is set to `Digital Pin Input`.
  2. Make sure `Pin Mode` is set to `Pull Up`.
  2. Click on the `Find Pin` button, and then tilt your guitar. If you have wired everything correctly, the tool should detect the pin and the tilt icon should light up whenever you tilt the guitar.
  3. If you are using a SW520D based tilt sensor, some versions of this sensor will have an inverted output. You can turn on the `Invert` option to correct this.
  </details>

  <details>
    <summary>Analog Tilt</summary>

  1. Click on Tilt, and make sure the `Input Type` is set to `Analog Pin Input`.
  2. Click on the `Find Pin` button, and then tilt your guitar. If you have wired everything correctly, the tool should detect the pin and the tilt `Original Value` value should change as you tilt your guitar.
  3. Click on `Calibrate`
  5. Hold your guitar in its resting position, and then click `Next`
  5. Tilt the guitar up, and then click `Next`
  6. Tilt your guitar up a little bit and then click `Next`. Values below this position will be zeroed.
  </details>
  <details>
     <summary>ADXL345 or MPU-6050 or LIS3DH based Tilt</summary>

   1. Click on `Add setting`
   2. Add an `Accelerometer`
   3. For the Pi Pico, set the SDA and SCL pins that you have used.
   4. Set the `Accelerometer Type` for your given sensor
   5. Click `Save`
   6. Click on Tilt, and make sure the `Input Type` is set to `Accelerometer Input`.
   7. Click on `Calibrate`
   8. Hold your guitar in its resting position, and then click `Next`
   9. Tilt the guitar up, and then click `Next`
   10. Tilt your guitar up a little bit and then click `Next`. Values below this position will be zeroed. With the adxl, you can increase the deadzone to help counteract strumming or shaking activating tilt.
   11. Adjust the `Low Pass Filter`. This value controls how new value from the ADXL are filtered, a value closer to 0 will result in a sensor that won't respond to a shake or strumming, but if you decrease it too much the sensor will have a decreased responsiveness. If you set it closer to 1, then the sensor will be very responsive but it will also pick up any tiny vibrations as well. From testing a value of 0.05 seemed like a good place to start. 
  </details>

  <details>
    <summary>MPR121</summary>

  1. Click on `MPR121 Settings`
  2. Enable the Peripheral
  3. Set the SDA and SCL pins that you have used.
  4. Set the `MPR121 Capacitive Touch Pad Count` based on the number of touch pads you are using. For a normal slider bar, this should be 5, or if you aren't using a slider bar set this to 0.
  5. Click `Save`
  </details>

  <details>
    <summary>Peripheral</summary>

  1. Click on `Peripheral Settings`
  2. Enable the Peripheral
  3. Set the SDA and SCL pins on the main PCB that are being connected to the peripheral.
  4. Click `Save`
  </details>

  <details>
    <summary>World Tour Slider Bar</summary>

  1. Click on Add Setting, and add a `Slider` setting
  2. Set the `Input Type` to `MPR121 Input`
  3. Click `Save Settings`. Note that everything else needs to be configured before you can do this.
  4. Set the channels based on your wiring. The configuration will update live, so if you are unsure which via maps to which slider pad, you can just work through the channels and assign things to the correct pads by touching each pad and working out which channel is which.
  5. Go to each fret, and set its `Input Type` to `MPR121 Input`.
  6. Set the channel based on your wiring. If you are unsure which channel you used, you can just walk through each channel and work out which one was used, as the status of the MPR121 will update live as you press and release frets.
  </details>

  <details>
    <summary>GH5 Neck (Standard wiring)</summary>

  1. The GH5 neck puts inputs over a single set of pins. This does add a bit of latency to the frets, but if you want the simplest wiring, you can keep the frets connected to the neck.
  2. Click on Add Setting, and add a `GH5 Neck Inputs` setting
  3. Set the SDA and SCL pins to the pins you used when you wired up the neck.
  4. If you would like to map the slider bar to the standard frets, you can enable "Slider to frets". Note that if you are using `Rock Band` mode, the tap bar is automatically mapped to the solo frets.
  5. Click on `enable` for each of the frets.
  </details>

  <details>
    <summary>Crazy Guitar Neck (Standard wiring)</summary>

  1. The Crazy Guitar neck puts inputs over a single set of pins. This does add a bit of latency to the frets, but if you want the simplest wiring, you can keep the frets connected to the neck.
  2. Click on Add Setting, and add a `Crazy Guitar Neck Inputs` setting
  3. Set the SDA and SCL pins to the pins you used when you wired up the neck.
  4. If you would like to map the slider bar to the standard frets, you can enable "Slider to frets". Note that if you are using `Rock Band` mode, the tap bar is automatically mapped to the solo frets.
  5. Click on `enable` for each of the frets.
  </details>

  <details>
    <summary>GH5 / Crazy Guitar (MPR121 Mode)</summary>

    1. If you wish to have better latency frets with these necks, you can replace the slider bar controller with an MPR121, which will give you low latency but will still use the same amount of wires.
    2. Click on Add Setting, and add a `Slider` setting
    3. Set the `Input Type` to `MPR121 Input`
    4. Click `Save Settings`. Note that everything else needs to be configured before you can do this.
    5. Set the channels based on your wiring. The configuration will update live, so if you are unsure which via maps to which slider pad, you can just work through the channels and assign things to the correct pads by touching each pad and working out which channel is which.
    6. Go to each fret, and set its `Input Type` to `MPR121 Input`.
    7. Set the channel based on your wiring. If you are unsure which channel you used, you can just walk through each channel and work out which one was used, as the status of the MPR121 will update live as you press and release frets.
  </details>

  <details>
    <summary>GH5 / Crazy Guitar (Peripheral Mode)</summary>

  1. If you wish to have better latency frets with these necks, but do not want to hardwire the frets, you can instead opt to use the peripheral mode. You can still follow the Standard wiring if you would like the slider bar to work, but then you do not need to enable the frets as we will bypass the original neck for those inputs.
  2. Go to each fret, and set its `Input Type` to `Digital Pin Input (Peripheral)`.
  2. Make sure `Pin Mode` is set to `Pull Up`.
  3. Use `Find Pin` to detect the pin your fret was hooked up to.
  </details>

  <details>
    <summary>USB Host (Pi Pico Only)</summary>

  1. Click on Add setting
  2. Find and add `USB Host inputs`
  3. Bind D+
  4. Click `Save`
  5. If you plug in a supported controller, the tool should detect it and tell you what it is.
  6. If you have a modded xbox and are using `usbdsecpatch`, you can disable `Authentication for Xbox 360`.

  </details>

14. If you do not want to hook an input up, click the `Remove` button to the right on that input.
15. Once everything is configured correctly, the `Save Settings` button should be clickable and you can hit that button to write your config to the guitar. It is now ready.
