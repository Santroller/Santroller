---
sort: 13
---

# Setting up emulation of a Wii extension (Pi Pico Only)

## Supplies

- A controller built using one of the other guides
* A USB Extension cable, or a USB breakout
- A Pi Pico
- A Wii extension plug
- A Schottky diode (a 20A diode should be plenty for this application, just needs to handle a >5V maximum reverse voltage)
- Some Wire
- A Soldering Iron  
- A boost converter that can step up 3.3v to 5V

## Steps


1. If you are using a USB extension cable, cut it in half and expose the four cables. Keep the socket end, as the goal is to plug a controller into this cable.
2. Hook up the V+ / VBUS (Red) to the VBUS pin on your Pi Pico
3. Hook up the V- / GND (Black) to ground on your Pi Pico
4. Hook up D+ (Green) to a unused digital pin.
5. Hook up D- (White) to the digital pin directly after D+. For example, you can hook up D+ to GP2 and D- to GP3.

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
    | Pi Pico (Advanced, Channel 0) | GP0, GP4, GP8, GP12, GP16, GP20  | GP1, GP5, GP9, GP13, GP17, GP21  |
    | Pi Pico (Advanced, Channel 1) | GP2, GP6, GP10, GP14, GP18, GP26 | GP3, GP7, GP11, GP15, GP19, GP27 |

2. Connect the V<sub>CC</sub> from the Wii Excension to the voltage input for your boost converter
3. Connect the voltage output from the boost converter to the pico, via the Schottky diode
4. Connect the ground pin from the boost converter to the pico
5. Connect the gnd pin to the gnd on your microcontroller.
6. If your plug doesn't already connect device detect to V<sub>CC</sub> inside the cable, connect device detect to V<sub>CC</sub>.

## Programming

1.  Start Santroller with your microcontroller plugged in.
3.  Hit Configure
4.  Click on `Add Setting`
5.  Find `Wii Extension Emulation` in the dropdown and add it 
6.  For the Pi Pico, set your SDA and SCL pins.
1.  Click on Add setting
2.  Find and add `USB Host inputs`
3.  Bind D+
4.  Hit Save
5.  If you plug in a supported controller to the usb port, the tool should detect it and tell you what it is. Most controllers are supported, but things like the play and charge kit won't work over USB.