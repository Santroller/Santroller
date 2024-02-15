---
sort: 3
---

# Setting up a Drum Kit

If you are new to this, read the [following guide](https://santroller.tangentmc.net/wiring_guides/general.html) for an overview on everything you need to know for modifying a controller.

## Supplies

- A microcontroller

  - The Pi Pico is recommended, but click below to see information about other microcontrollers.
    {% include sections/microcontrollers.md %}

- A 4051 or 4067 based analog multiplexer (Only necesssary for the Pi Pico, as other microcontrollers will have enough analog pins for a drum kit)
- 1MOhm resistor per drum / cymbal pad
- Some Wire
- Soldering Iron
- Multimeter (it will be used mainly in continuity mode, where it beeps when the two contacts are shorted together)
- Wire Strippers
- Wire Cutters
- Heatshrink

- Supplies for specific features
  - Drum kit from scratch
    - 1 Piezo sensor per drum pad / cymbal
  - USB Host
    - Pi Pico
    - One of the following
      - A USB female breakout 
      - A USB extension cable
      - A controller with a cable already attached that you are willing to cut

```note
Close the tool if you want to use your drum kit, the multiplexer is overridden by the tool when it updates its sensor values, and this will override the controller inputs when it happens.
```

## Wiring Steps
{% include sections/getting_started.md %}

To keep the relevant information in this guide easy to find, information is sorted by function.

<details>
    <summary>Face buttons</summary>

1. Trace the pads on the drum PCB that contains the face buttons. The face buttons should have a common ground, you should see this as a trace that connects multiple buttons together. Find some way to connect a wire to this, if there is a test pad or something you can solder to, solder your wire to that, otherwise you will need to scrape back one of the traces with a knife so you can solder to that. Solder this to ground on your microcontroller.
2. Follow the traces for the other side of each button, and solder a wire to them in a similar way as the common wire. Then solder that to a digital pin on your microcontroller.

</details>

<details>
    <summary>Multiplexer</summary>

[![4051](/assets/images/cd4051.png)](/assets/images/cd4051.png)
[![4067](/assets/images/cd4067.png)](/assets/images/cd4067.png)

1. Wire V<sub>DD</sub> on the multiplexer to 3v3 on the Pico.
2. Wire V<sub>SS</sub> to GND on your Pico
3. If your multiplexer has a V<sub>EE</sub>, also wire that to ground on the Pi Pico. V<sub>EE</sub> allows for using the multiplexer with negative voltages, but since we aren't doing this we set it to ground to disable that feature.
4. Also wire INH / Inhibit to ground, if this exists on your multiplexer. This pin disables the I/O if it is driven high, so we ground it to make sure the chip is always enabled.
5. Wire the analog output (Often labelled COM or common in/out, but also labelled SIG on some breakout boards) on the multiplexer to an analog pin on your Pi Pico.
6. Wire A/S0, B/S1 and C/S2 (and D/S3 for the 16 channel multiplexer) to seperate digital pins on your Pi Pico.
7. Wire each drum pad to a different channel on the multiplexer.

</details>

<details>
    <summary>Drum / Cymbal pads</summary>

1. Disconnect the piezos from the main drum PCB.
2. Solder the black wire from the piezo to ground.
3. Solder the red wire to an analog input on the multiplexer for the Pi Pico, or to an analog pin on your micocontroller if your microcontroller has enough analog pins.
4. Solder a 1Mohm resistor between the red and black wires on the piezo.

</details>

<details>
    <summary>RB Pedal</summary>

1. The pedal connector has two wires coming out of it. Connect one to ground and one to a digital pin on your microcontroller.

</details>

<details>
    <summary>GH Pedal</summary>

1. Solder the one wire from the pedal connector to ground.
2. Solder the other wire to an analog input on the multiplexer for the Pi Pico, or to an analog pin on your micocontroller if your microcontroller has enough analog pins.
3. Solder a 1Mohm resistor between the two wires on the pedal connector.

</details>

<details>
    <summary>USB Host (Pi Pico Only)</summary>
If you want to use your controller on an unmodifed Xbox 360 or Xbox One or Xbox Series, you can wire a USB port to the Pi Pico.

1. If you are using a USB extension cable, cut it in half and expose the four cables.
2. Hook up the V+ (Red) to the VBUS pin on your Pi Pico
3. Hook up the V- (Black) to ground on your Pi Pico
4. Hook up D+ (Green) to a unused digital pin.
5. Hook up D- (White) to the digital pin directly after D+. For example, you can hook up D+ to GP2 and D- to GP3.

</details>

## Programming

1.  Start Santroller with your microcontroller plugged in.
2.  Set the Input Type to Directly Wired
3.  Hit Configure
4.  Click on `Controller Settings`
5.  Set `Emulation Type` to `Controller` for standard guitars and `Fortnite Festival` for Fortnite Festival.
6.  Set the `Controller Type` based on the game you want to play.
7.  `Windows controller mode` can be set based on your preferences. Note that this only affects windows, a controller in XInput mode will use the correct mode on a console, and will automatically use HID mode on Linux and macOS.
    1. `XInput` - This works more natively on windows, and most games will automatically bind controls.
    2. `HID` - This uses HID on windows, which means games won't automatically bind controls, but HID is polled a bit more efficiently in games like Clone Hero.
8.  If you would like to adjust settings related to polling, click on `Controller Poll Settings`
9.  Set the Poll Rate to your preferred setting. 0 sends data as fast as possible, any other number polls inputs at that speed.
10. Debounce can be adjusted here. You will need to increase this, as debounce sets the minimum time for a drum hit, and this will turn the drum hit that only occurs for a few microseconds into a signal that the game expects. While testing I used a debounce of 20ms but you will have to adjust this to find a value that works for you. The main thing to consider is that it takes a bit of time between you going to hit a drum pad and you hitting the pad, so if the debounce is too low, you may just miss drum hits due to the note not being on for long enough.
11. Now you can start setting up your inputs. To keep this information relevant, it is grouped by function.
    <details>
      <summary>Face buttons / RB Pedal</summary>

    1. Click on the button you want to configure, and make sure the `Input Type` is set to `Digital Pin Input`.
    2. Click on the `Find Pin` button, and then press the button on the guitar. If you have wired everything correctly, the tool should detect the pin and the icon for that button should now light up whenever the button is pressed.

    </details>

    <details>
      <summary>Drum / Cymbal pads / GH Pedal (Multiplexer)</summary>

    1. Click on the drum pad in question
    2. Set the `Input Type` to multiplexer
    3. Set the S0, S1, S2 (and S3 for a 4067 based multiplexer) pins
    4. Set the analog output / SIG / COM pin to the analog pin you wired it to
    5. Hit the drum pad in question. You should see the raw value for the drum pad change.
    6. Hit the pad lightly, and drag the minimum up so that it registers small hits.
    7. Hit nearby pads and make sure that the vibrations from those pads don't activate the pad you are configuring. If they do, then increase the minimum. You should end up with each pad registering hits, without crosstalk or needing to hit the pads too hard.

    </details>

    <details>
      <summary>Drum / Cymbal pads / GH Pedal (Direct analog)</summary>

    1. Click on the drum pad in question
    2. Set the input type to `Analog Pin Input`
    3. Click on `Find Pin` and then hit the drum pad in question, it should detect the drum that was just hit.
    4. Hit the drum pad in question. You should see the raw value for the drum pad change.
    5. Hit the pad lightly, and drag the minimum up so that it registers small hits.
    6. Hit nearby pads and make sure that the vibrations from those pads don't activate the pad you are configuring. If they do, then increase the minimum. You should end up with each pad registering hits, without crosstalk or needing to hit the pads too hard.

    </details>

    <details>
      <summary>USB Host (Pi Pico Only)</summary>

    1. Click on Add setting
    2. Find and add `USB Host inputs`
    3. Bind D+
    4. Hit Save
    5. If you plug in a supported controller, the tool should detect it and tell you what it is.
    6. If you have a modded xbox and are using `usbdsecpatch`, you can disable `Authentication for Xbox 360`. 

    </details>
