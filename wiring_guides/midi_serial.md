---
sort: 7
---

# Setting up an Adaptor for a Standard MIDI device (Pro Keys / Drums) (Pi Pico Only)

## Supplies

- A Raspberry Pi Pico
- H11L1 optoisolator
- 1N914 diode
- 1x 220Ω resistor
- 1x 470Ω resistor
- 100nF capacitor
- Midi Socket

## Wiring Steps

[![MIDI Pinout](/assets/images/diy-midi-receive-3.3v-v2_schem-1.png?w=2000&h=&ssl=1)](/assets/images/diy-midi-receive-3.3v-v2_schem-1.png?w=2000&h=&ssl=1)

1. Wire up the components according to the above diagram.
2. Wire the RX pin to your Pico. Use one of the following pins:

   | Microcontroller       | RX                              |
   | --------------------- | ------------------------------- |
   | Pi Pico (Recommended) | GP5                             |
   | Pi Pico (Advanced)    | GP1, GP5, GP9, GP13, GP17, GP21 |

## Configuring

1. Click on `Add Setting` on left of Santroller Window
2. Find and add `MIDI inputs`
3. Set the Serial MIDI pin to the RX pin you used.
4. Click `Save`
5. For Pro Keys, you will need to pick what key your keyboard starts at. You can click on `Click to assign`, then press a note on your keyboard. That will be mapped to the Pro Keys note, with every other note being assigned to the next notes in order.
6. If you plug in a supported controller, the tool should detect it and tell you what it is. Most controllers are supported, but things like the play and charge kit won't work over USB.
