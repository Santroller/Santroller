---
sort: 13
---

# Setting up emulation of a Wii extension

## Supplies

- A controller built using one of the other guides

- A microcontroller

  - The Pi Pico is recommended, but other 3.3v microcontrollers like the 3.3v pro micro may work. 5v only microcontrollers will NOT work.

- A Wii extension plug
- Some Wire
- A Soldering Iron

```danger
NEVER plug in both the usb cable to your computer, and the extension to a wii remote, it can only ever be plugged into one device at a time!
```

## Steps

1.  Connect wires between the SDA and SCL pins on your breakout board / wii extension cable.
    Refer to the following image for the pinout of a Wii Extension connector.

    The Pi Pico lets you pick from various pins for the SDA and SCL pins. We provide recommended pins below, and this pinout is the same as the old Ardwiino firmware. If you need to use other pins, the options are provided below but the SDA and SCL pins must be from the same channel.

    [![pinout](/assets/images/wii.png)](/assets/images/wii.png) [![Finished adaptor](/assets/images/wii-ext.jpg)](/assets/images/wii-ext.jpg)

    ```danger
    If you are using a wii extension cable do NOT rely on the colours, the manufacturers are all over the place with this and the only way to validate them is to test each wire according to the above image. I've come across connectors wired with green as ground and black as 3.3V before, you just can't rely on the colours at all unfortunately.
    ```

    | Microcontroller               | SDA                              | SCL                              |
    | ----------------------------- | -------------------------------- | -------------------------------- |
    | Pi Pico (Recommended)         | GP18                             | GP19                             |
    | Pro Micro, Leonardo, Micro    | 2                                | 3                                |
    | Pi Pico (Advanced, Channel 0) | GP0, GP4, GP8, GP12, GP16, GP20  | GP1, GP5, GP9, GP13, GP17, GP21  |
    | Pi Pico (Advanced, Channel 1) | GP2, GP6, GP10, GP14, GP18, GP26 | GP3, GP7, GP11, GP15, GP19, GP27 |

2.  Connect the V<sub>CC</sub> on the microcontroller to the V<sub>CC</sub> on the breakout
    - On the Pi Pico, use V<sub>SYS</sub>
    - On the 3.3V Pro Micro, use V<sub>CC</sub>
3.  Connect the gnd pin to the gnd on your microcontroller.

## Programming

1.  Start Santroller with your microcontroller plugged in.
3.  Hit Configure
4.  Click on `Add Setting`
5.  Find `Wii Extension Emulation` in the dropdown and add it 
6.  For the Pi Pico, set your SDA and SCL pins.