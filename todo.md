# Things to work on

## Firmware features
- [ ] bluetooth timeout
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
- [ ] max1704x
- [ ] battery level estimate via ADC pin

# things missing from S1

Missing: power and timing

Sleep after inactivity, with a wake pin. This is the "put controller to sleep" and "bluetooth timeout" items in todo.md, and the old firmware already did it.
LED inactivity timeout. The new LED guide in the configurator already tells users to set an inactivity timer, but nothing implements one.
Global poll rate.
Global button and strum debounce. Debounce is per mapping only now.
Combined strum debounce, where up and down share one window.

The reset/reboot binding: a button that reboots the controller and keeps the console type.

"Select → D-pad Left on Xbox One / PS4" for 5-fret guitars.

Num/Caps/Scroll Lock LEDs.
Missing: LEDs

Santroller game-feedback LED commands: note hit and miss, star power, multiplier, solo. The handlers are commented out in instance.cpp.
Status LEDs: Bluetooth connected, console auth complete, and current mode.
LEDs on MPR121 GPIO pins.

Accelerometer low-pass filter.
MAX1704x battery level reported over Bluetooth.
A drum sensitivity knob, setting the hit threshold live.

Heartbeat/inactivity pulse outputs.
The old USB-host detection of Xbox One controllers running in XInput-compatible mode.