---
sort: 13
---

# Setting up emulation of a Wii extension

```danger
If you wish to also use USB Host, follow the [Wii extension emulation + USB Host guide](https://santroller.tangentmc.net/wiring_guides/wii_output_usb_host.html)
```


## Supplies

- A controller built using one of the other guides
- A microcontroller

  - The Pi Pico is currently the only supported microcontroller. You will need to use an [older build](https://github.com/Santroller/Santroller/releases/tag/v10.1.188) if you wish to use any of the other microcontrollers listed below, and no support will be provided as this build is very old. This build is also missing quite a few features from the more recent builds, and in the future Santroller will only support the Pi Pico, or other boards based on it.

    {% include sections/microcontrollers.md %}

- A Wii extension plug
- Some Wire
- A Soldering Iron

## Steps

1.  Connect wires between the SDA and SCL pins on your wii extension plug.
    Refer to the following image for the pinout of a Wii Extension plug.

    The Pi Pico lets you pick from various pins for the SDA and SCL pins. We provide recommended pins below, and this pinout is the same as the old Ardwiino firmware. If you need to use other pins, the options are provided below but the SDA and SCL pins must be from the same channel.

    [![pinout](/assets/images/wii-ext-pinout.png)](/assets/images/wii-ext-pinout.png)

    ```danger
    Do NOT rely on the colours, the manufacturers are all over the place with this and the only way to validate them is to test each wire according to the above image. I've come across connectors wired with green as ground and black as 3.3V before, you just can't rely on the colours at all unfortunately. Even cables from guitar hero guitars don't use expected colours, I found that on mine, device detect was shorted to V<sub>CC</sub>, and then white was ground!
    ```

    | Microcontroller               | SDA                              | SCL                              |
    | ----------------------------- | -------------------------------- | -------------------------------- |
    | Pi Pico (Recommended)         | GP18                             | GP19                             |
    | Pro Micro, Leonardo, Micro    | 2                                | 3                                |
    | Pi Pico (Advanced, Channel 0) | GP0, GP4, GP8, GP12, GP16, GP20  | GP1, GP5, GP9, GP13, GP17, GP21  |
    | Pi Pico (Advanced, Channel 1) | GP2, GP6, GP10, GP14, GP18, GP26 | GP3, GP7, GP11, GP15, GP19, GP27 |

2.  Connect the V<sub>CC</sub> on the microcontroller to the V<sub>CC</sub> on the plug, through the diode
    - On the Pi Pico, use V<sub>SYS</sub>
    - On the 3.3V Pro Micro, use V<sub>CC</sub>
3.  Connect the gnd pin to the gnd on your microcontroller.
4.  If your plug doesn't already connect device detect to V<sub>CC</sub> inside the cable, connect device detect to V<sub>CC</sub>.

## Programming

1.  Start Santroller with your microcontroller plugged in.
3.  Hit Configure
4.  Click on `Add Setting`
5.  Find `Wii Extension Emulation` in the dropdown and add it 
6.  For the Pi Pico, set your SDA and SCL pins.

## USB Host

Make sure you follow the relevant steps outlined in the [USB Host Guide](https://santroller.tangentmc.net/wiring_guides/usb.html) as some extra wiring is necessary to power USB devices when using Wii Extension Emulation.
