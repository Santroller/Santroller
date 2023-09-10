# PS3 - Standard Controller
## HID Report Layout
The HID Report layout is below:
```
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|            buttons            |      hat      |   left_joy_x  |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|   left_joy_y  |  right_joy_x  |  right_joy_y  |  axis_dpad_up |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|axis_dpad_right| axis_dpad_left| axis_dpad_down|    axis_l2    |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|    axis_r2    |    axis_l1    |    axis_l2    | axis_triangle |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|  axis_circle  |   axis_cross  |  axis_square  |accelerometer_x|
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|               |        accelerometer_z        |accelerometer_y|
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|               |           gyroscope           |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

### Buttons
Buttons are broken up as follows, with one bit per button

```
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|     square    |     cross     |     circle    |    triangle   |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|       l2      |       r2      |       l1      |       r1      |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|     select    |     start     | left_stick_in | right_stick_in|
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|       ps      |
+-+-+-+-+-+-+-+-+
```

### POV Hat (DPad)
The hat values look like the below diagram:

```
 7  0  1
  \ | /
6 - 8 - 2
  / | \
 5  4  3
```

For example, `0x08` is returned when holding no buttons, and holding both left and up results in a value of `0x01`.

### Pressure Axes
All face buttons are pressure sensitive and can measure a value from 0x00 being not pressed, to 0xFF being fully pressed. Triggers are handled here too.

### Accelerometers and Gyro
Accelerometer and gyroscope are 10 bit numbers: nominally, they are neutral at `0x200`, and range from `0x000` to `0x400`. The actual neutral/min/max varies wildly between real controllers though, calibration is required for proper usage in games.

From a resting position, X measures left/right acceleration (roll), Y measures forward/back acceleration (pitch), Z measures up/down acceleration (gravity), and the gyroscope measures instantaneous left/right rotation (yaw).

## ID Control Request
For making the PS button work, there are some extra requirements. When the device receives a control request with a `bmRequestType` of `device to host, class and interface`, a `bmRequest` of `HID_GET_REPORT (0x01)`, and a `wValue` of `0x0300` (feature report), it needs to respond with the following bytes: 
```
 0                   1                   2                   3  
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|      0x21     |      0x26     |      0x01     |   ID (0x07)   |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|      0x00     |      0x00     |      0x00     |      0x00     |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

## VID and PID
Note that a specific VID and PID is not required at all for the controller to work.
