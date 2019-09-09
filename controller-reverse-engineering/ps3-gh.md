#Guitar Hero Controller:
Most things about the controller are similar to the standard controller, except the buttons and axis have different meanings

## Hid Report
The HID Report layout is below:

```
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|            buttons            |      hat      |   left_joy_x  |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|   left_joy_y  |     whammy    |    tap_bar    |axis_dpad_right|
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
| axis_dpad_left|  axis_dpad_up |   dpad_down   |  axis_orange  |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|  axis_yellow  |    axis_red   |   axis_green  |   axis_blue   |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|    axis_r2    |    axis_l1    |    axis_r1    |      tilt     |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|               |        accelerometer_y        |accelerometer_z|
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|               |           gyroscope           |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

### Tilt
Note that tilt requires both bits, and goes between 0x0184 and 0x01f7. However, the original guitar just jumps between these values,
so it is sufficient to use 0x0184 as a neutral position, and 0x01f7 as a tilted position.

### Buttons
The buttons also change slightly from a normal controller.Note that blue and yellow are switch when compared to a Rockband controller!

```
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|      blue     |      red      |     green     |     yellow    |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|     orange    |       r2      |       l1      |       r1      |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|     select    |     start     | left_stick_in | right_stick_in|
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|       ps      |
+-+-+-+-+-+-+-+-+
```
### Hat
Note that for the hat, 0x1f is returned when nothing is held, not 0x08.

## VIDs and PIDs
Note that it is a requirement to use the GH vids and pids for the game to detect your controller, which are:

| Device | VID    | PID    |
| ------ | ------ | ------ |
| Guitar | 0x12ba | 0x0100 |
| Drum   | 0x12ba | 0x0120 |

## ID Control Request
The ID Control Request changes slightly, as we use an id of 0x06, not 0x07. The revised request is below: Note that without this change, the tilt axis will not work.
```
 0                   1                   2                   3  
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|      0x21     |      0x26     |      0x01     |      0x06     |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|      0x00     |      0x00     |      0x00     |      0x00     |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

A LUFA implementation of this controller is in [output_ps3.c](/src/shared/output/output_ps3.c)