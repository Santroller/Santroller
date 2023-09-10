# PS3 - Guitar Hero Guitar Controller
Most things about the controller are similar to the standard controller, except the buttons and axis have different meanings.

Interestingly, the PS3 GH guitar has some notable differences compared to other rhythm controllers.

## HID Report Layout

```
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|            buttons            |      hat      |    padding    |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|    padding    |     whammy    |    tap_bar    |axis_dpad_right|
|               |               |               |  axis_yellow  |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
| axis_dpad_left|  axis_dpad_up | axis_dpad_down|   axis_blue   |
|               |   axis_green  |  axis_orange  |               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|   axis_red    |    padding    |    padding    |    padding    |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|    padding    |    padding    |    padding    |accelerometer_x|
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|               |        accelerometer_z        |accelerometer_y|
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|               |    padding    |    padding    |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

### Buttons
The buttons also change slightly from a normal controller. Note that blue and yellow are switched when compared to the other rhythm controllers!

```
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|     yellow    |     green     |      red      |      blue     |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|     orange    |     pedal     |    padding    |    padding    |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|     select    |     start     |    padding    |    padding    |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|       ps      |
+-+-+-+-+-+-+-+-+
```
### Hat
Note that for the hat, `0x1f` is returned when nothing is held, not `0x08`.

### Axis Fields
For the fields with multiple buttons, pressing both buttons will return a value of 0, while only pressing one of the two buttons gives 0xFF. For every other axis, pressing the button returns 0xFF.

### Tilt
Accelerometer X is the axis used for typical tilt. The other two axes are unneeded and can be ignored if you're not interested in them.
For the PS3, accelerometer data is 10 bit, and when the guitar is perfectly horizontal, it rests at 475. The tilt ranges from 384 to 640 (512 &plusmn; 128), and will wrap around when you tilt the guitar too far down.

## VID and PID
Note that it is a requirement to use the following VID and PID for the game to detect your controller as a guitar:

| VID      | PID      |
| -------- | -------- |
| `0x12ba` | `0x0100` |

## ID Control Request
The ID Control Request changes slightly, as we use an ID of `0x06`, not `0x07`. The revised request is below:
```
 0                   1                   2                   3  
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|      0x21     |      0x26     |      0x01     |   ID (0x06)   |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|      0x00     |      0x00     |      0x00     |      0x00     |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

Note that without this change, the tilt axis will not work.
