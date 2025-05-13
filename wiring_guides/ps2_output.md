---
sort: 14
---

# Setting up emulation of a PS2 controller

```danger
If you wish to also use USB Host, follow the [PS2 controller + USB Host guide](https://santroller.tangentmc.net/wiring_guides/ps2_output_usb_host.html)
```

```danger
Be extremely careful with the rumble wire. 

If your connector has a rumble wire we recommend you make sure it is properly covered up, as you can easily blow the rumble fuse inside the console if the rumble wire touches something it shouldnt!
```

## Supplies

- A controller built using one of the other guides

- A microcontroller

  - The Pi Pico is recommended, but other 3.3v microcontrollers like the 3.3v pro micro may work. 5v only microcontrollers will NOT work.

- A Schottky diode (a 20A diode should be plenty for this application, just needs to handle a >10V maximum reverse voltage)
  - While this isn't strictly necessary, it will protect you from plugging in both the PS2 and USB at the same time, and frying your PS2.

- A PS2 controller plug
- Some Wire
- A Soldering Iron

## Steps

1. Connect pins according to the below table.

   For beginners we provide a recommended pinout below for each microcontroller.
   If these pins don't work for you, you can use one of the advanced pinouts below. Note that the Pi Pico has multiple channels, and SCK, MOSI and MISO need to be from the same channel.

   [![Adapter pinout](/assets/images/ps2-pinout.png)](/assets/images/ps2-pinout.png)

   | Microcontroller                          | SCK / Clock / Blue | MOSI / Command / Orange | MISO / Data / Brown | Acknowledge / Green | Attention / Yellow |
   | ---------------------------------------- | ------------------ | ----------------------- | ------------------- | ------------------- | ------------------ |
   | Pi Pico (Recommended)                    | GP6                | GP4                     | GP3                 | GP7                 | GP10               |
   | Pro Micro, Leonardo, Micro (Recommended) | 15                 | 16                      | 14                  | 7                   | 10                 |
   | Uno (Recommended)                        | 13                 | 11                      | 12                  | 2                   | 10                 |
   | Mega (Recommended)                       | 52                 | 51                      | 50                  | 2                   | 10                 |
   | Pi Pico (Advanced)                       | Any                | Any                     | Any                 | Any                 | Any                |
   | Pro Micro, Leonardo, Micro (Advanced)    | 15                 | 16                      | 14                  | Any                 | Any                |
   | Uno (Advanced)                           | 13                 | 11                      | 12                  | Any                 | Any                |
   | Mega (Advanced)                          | 52                 | 51                      | 50                  | Any                 | Any                |

2. Connect the V<sub>CC</sub> on the microcontroller to the V<sub>CC</sub> on the plug, through the diode
   - On the Pi Pico, use V<sub>SYS</sub>
   - On the 3.3V Pro Micro, use V<sub>CC</sub>
3. Connect the gnd pin to the gnd on your microcontroller.

## Programming

1.  Start Santroller with your microcontroller plugged in.
2.  Hit Configure
3.  Click on `Add Setting`
4.  Find `PS2 Controller Emulation` in the dropdown and add it
5.  For the Pi Pico, set your SCK, MOSI, and MISO pins
6.  Set your Acknowledge and Attention pins
