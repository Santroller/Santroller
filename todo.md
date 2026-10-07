# Things to work on

## Firmware features
- [ ] input to put controller to sleep
- [ ] When a bt receivers USB is suspended, we should give users the option to suspend BT itself as well.

## Emulation
- [ ] Emulation of N64 controllers
- [ ] Emulation of gamecube controllers
- [ ] Emulation of SNES controllers
- [ ] Emulation of NES controllers
- [ ] Emulation of MIDI devices
- [ ] Emulation of xinput over bt
- [ ] Emulation of fight sticks
- [ ] Emulation of flight sticks
- [ ] Emulation of wheels
- [ ] Emulation of microphones
- [ ] Emulation of GCN adapters

## Inputs
- [ ] Input from N64 controllers
- [ ] Input from gamecube controllers
- [ ] Input from SNES controllers
- [ ] Input from NES controllers
- [ ] Inputs from NKRO keyboards over usb host

## Mapping features
- [ ] Reverse debounce - https://github.com/Santroller/Santroller/issues/101
- [ ] Better analog filtering - https://github.com/dxinteractive/ResponsiveAnalogRead
- [ ] Rapid Trigger - https://github.com/minipadKB/minipad-firmware/blob/master/src/handlers/key_handler.cpp
- [ ] Other analog filters - Low pass, high pass, zero point crossing
- [ ] Filters needed for hall effect

## Led / rumble features
- [ ] Allow mapping stage kit led count to brightness
- [ ] Support directoutput (if we want to start supporting pinball cabs)

## Devices
- [ ] Sound for PS4/5, XInput and GIP
- [x] max1704x
- [x] battery level estimate via ADC pin

# things missing from S1


Accelerometer low-pass filter.
A drum sensitivity knob, setting the hit threshold live.

The old USB-host detection of Xbox One controllers running in XInput-compatible mode.