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


From s1:
Here's what the old stack (Santroller 1 firmware plus the C# configurator) had that the new one is missing or only partly has. I checked two of the bugs myself; the rest is from an agent comparing the two codebases, and I haven't verified each item.

Bugs found along the way


Mouse emulation is broken. MouseButtonMapping and MouseAxisMapping write a mouse report into the keyboard device's buffer, so a left click probably comes out as Left Ctrl. The scroll X/Y fields also look swapped.

Missing: power and timing

Sleep after inactivity, with a wake pin. This is the "put controller to sleep" and "bluetooth timeout" items in todo.md, and the old firmware already did it.
LED inactivity timeout. The new LED guide in the configurator already tells users to set an inactivity timer, but nothing implements one.
Global poll rate.
Global button and strum debounce. Debounce is per mapping only now.
Combined strum debounce, where up and down share one window.
Missing: console modes

Fortnite Festival:

the PC Rock Band guitar mode;
the PC Festival Pro gamepad layer;
the iOS Festival mode;
the menu/gameplay layer toggle binding;
the Festival keyboard presets.
Only the Switch Festival layer exists now.

The reset/reboot binding: a button that reboots the controller and keeps the console type.

RPCS3: switching to PS3 mode for RPCS3 is now always on, with no opt-out, and the RPCS3 whammy fix is a TODO.

"Select → D-pad Left on Xbox One / PS4" for 5-fret guitars.

Missing: keyboard and mouse

Working mouse emulation: the new keyboard device only has a keyboard descriptor and report. Absolute vs relative mouse mode is also missing.
Media keys (volume, play/pause and so on).
NKRO: the new keyboard is 6KRO only.
Num/Caps/Scroll Lock LEDs.
Missing: LEDs

Santroller game-feedback LED commands: note hit and miss, star power, multiplier, solo. The handlers are commented out in instance.cpp.
Status LEDs: Bluetooth connected, console auth complete, and current mode.
LEDs on MPR121 GPIO pins.
Any LED, rumble or output pins on the secondary (peripheral) Pico: its link only carries config and OTA now.
Inverted (active-low) GPIO LED pins: possibly missing; the agent wasn't sure.
Missing: inputs and peripherals

The direct-wired GHWT tap-bar neck, and its sensitivity setting.
Standalone DJ Hero platters: the device exists but there's no input type, so they can't be mapped. Also missing are platter smoothing (a 16-sample average) and a platter poll rate.
Accelerometer low-pass filter.
MAX1704x battery level reported over Bluetooth.
A drum sensitivity knob, setting the hit threshold live.

Heartbeat/inactivity pulse outputs.
A 10ms report throttle for PS3 DJ Hero.
The old USB-host detection of Xbox One controllers running in XInput-compatible mode.