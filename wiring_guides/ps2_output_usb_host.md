---
sort: 14
---

# Setting up emulation of a PS2 controller with USB Host (Pi Pico Only)

## Supplies

- A controller built using one of the other guides
* A USB Extension cable, or a USB breakout
- A Pi Pico
- A PS2 controller plug
- A Schottky diode (a 20A diode should be plenty for this application, just needs to handle a >10V maximum reverse voltage)
- Some Wire
- A Soldering Iron
- Either:
  - Some form of 5V regulator that can regulate >8V down to 5V
    - Note that this method uses the rumble line to power the microcontroller, so your plug will need to have that exposed
    - PS2 Guitars often do NOT have this exposed, so this method will not work in that scenario
  - A boost converter that can step up 3.3v to 5V
    - This method uses the 3.3v rail on the controller

## Steps

1. If you are using a USB extension cable, cut it in half and expose the four cables. Keep the socket end, as the goal is to plug a controller into this cable.
2. Hook up the V+ / VBUS (Red) to the VBUS pin on your Pi Pico
3. Hook up the V- / GND (Black) to ground on your Pi Pico
4. Hook up D+ (Green) to a unused digital pin.
5. Hook up D- (White) to the digital pin directly after D+. For example, you can hook up D+ to GP2 and D- to GP3.
1. Connect pins according to the below table.

   For beginners we provide a recommended pinout below for each microcontroller.
   If these pins don't work for you, you can use one of the advanced pinouts below. Note that the Pi Pico has multiple channels, and SCK, MOSI and MISO need to be from the same channel.

   [![Adapter pinout](/assets/images/ps2-pinout.png)](/assets/images/ps2-pinout.png)

   | Microcontroller                          | SCK / Clock / Blue | MOSI / Command / Orange | MISO / Data / Brown | Acknowledge / Green | Attention / Yellow |
   | ---------------------------------------- | ------------------ | ----------------------- | ------------------- | ------------------- | ------------------ |
   | Pi Pico (Recommended)                    | GP6                | GP4                     | GP3                 | GP7                 | GP10               |
   | Pi Pico (Advanced)                       | Any                | Any                     | Any                 | Any                 | Any                |

2. Connect the gnd pin to the gnd on your microcontroller.

3. Pick one of the following options depending on the type of voltage converter you wish to use
   1. If you are using a 5V regulator:
      1. Connect the rumble wire (grey) from the PS2 plug to the voltage input for your regulator
      2. Connect the voltage output from the regulator to the pico, via the Schottky diode
      3. Connect the ground pin from the regulator to the pico
   2. If you are using a 3.3V to 5V boost converter:
      1. Connect the 3.3V wire (red) from the PS2 plug to the voltage input for your boost converter
      2. Connect the voltage output from the boost converter to the pico, via the Schottky diode
      3. Connect the ground pin from the boost converter to the pico

## Programming

1.  Start Santroller with your microcontroller plugged in.
2.  Hit Configure
3.  Click on `Add Setting`
4.  Find `PS2 Controller Emulation` in the dropdown and add it
5.  For the Pi Pico, set your SCK, MOSI, and MISO pins
6.  Set your Acknowledge and Attention pins
1.  Click on Add setting
2.  Find and add `USB Host inputs`
3.  Bind D+
4.  Hit Save
5.  If you plug in a supported controller to the usb port, the tool should detect it and tell you what it is. Most controllers are supported, but things like the play and charge kit won't work over USB.
