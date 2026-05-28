# Things to work on

## Tool features
[ ] Add a dialog that warns a user that the guitar needs a reboot to reload changes
[ ] Add an assignment for when the controller times out / goes to sleep, then people can choose what leds stay on

## Firmware features
[ ] bluetooth timeout
[ ] max1704x battery levels
[ ] battery levels by just measuring the voltage from an ADC pin
[ ] input to put controller to sleep
[ ] input for waking the controller up from sleep

## Emulation
[ ] Emulation of PS2 controllers
[ ] Emulation of Wii extensions
[ ] Emulation of Wii remotes
[ ] Emulation of N64 controllers
[ ] Emulation of gamecube controllers
[ ] Emulation of SNES controllers
[ ] Emulation of NES controllers
[ ] Emulation of MIDI devices
[ ] Emulation of xinput over bt
[ ] Emulation of fight sticks
[ ] Emulation of flight sticks
[ ] Emulation of wheels
[ ] Emulation of keyboard / mouse
[ ] Emulation of microphones
[ ] Emulation of GCN adapters

## Inputs
[ ] Input from N64 controllers
[ ] Input from gamecube controllers
[ ] Input from SNES controllers
[ ] Input from NES controllers
[ ] Input from bluetooth controllers (via bluepad?)
[ ] Inputs from NKRO keyboards over usb host
[ ] Inputs from switch controllers over usb host

## Mapping features
[ ] Defaults for inputs, such as loading all the wii extension inputs for you with one click
[ ] Defaults for outputs, like clicking a button that gives you all the keyboard inputs youd need for festival
[ ] Reverse debounce - https://github.com/Santroller/Santroller/issues/101
[ ] Copy inputs across profiles
[ ] Copy profiles
[ ] Advanced joystick mapping - allow for mapping to just a specific range of motion instead of the entire stick. Some games need the ability to map an axis to only the top half of a stick for example
[ ] Cycling mapping - you give it a range of values (like say all the notches on a rb pickup), and then each time an input is pressed, it jumps to the next value, cycling back to the beginning after it hits the last one
    For this i think the easiest method for implementation would be to have a seperate config area we store that state in
[ ] Pin labels - Add the ability to define names for pins, so that a tool like RCM can let users reconfigure their guitars, but the pins are all given labels so a user knows what the defaults are
[ ] Better analog filtering - https://github.com/dxinteractive/ResponsiveAnalogRead
[ ] Rapid Trigger - https://github.com/minipadKB/minipad-firmware/blob/master/src/handlers/key_handler.cpp
[ ] Other analog filters - Low pass, high pass, zero point crossing
[ ] Filters needed for hall effect
[ ] Shortcut input masking - aka if you have start + select to home, it shouldnt keep pressing start and select

## Led / rumble features
[ ] Gamepad rumble, including passthrough with usb host
[ ] DMX
[ ] Allow mapping stage kit led count to brightness
[ ] Support Spice2X serial API for games that run on that
[ ] Support directoutput (if we want to start supporting pinball cabs)

## Devices
[ ] Xbox 360 slim RF module - https://github.com/ginokgx/xbox360slimRF
[ ] Xbox 360 fat RF module - https://github.com/blackfin/xbox360RFmodule
[ ] Sound for PS4/5, XInput and GIP
[ ] Port over logic for waking up 360s
[ ] Battery levels