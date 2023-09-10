# Switch - Hori Pad
The Switch Hori pad functions similarly to a PS3 controller, with some notable differences.

## HID report
The pressure and accelerometer bytes are not included in the report, though the implementation used here disregards that and uses the same report as the PS3 controller.

```
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|            buttons            |      hat      |   left_joy_x  |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|   left_joy_y  |  right_joy_x  |  right_joy_y  | vendor-defined|
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

### Buttons
Some buttons appear swapped around relative to their PS3 counterparts, but the bits used are exactly the same in terms of physical position. The Capture button is also added to these bits.

```
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|       y       |       b       |       a       |       x       |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|       l       |       r       |       zl      |       zr      |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|     minus     |      plus     | left_stick_in | right_stick_in|
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|      home     |    capture    |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

## VIDs and PIDs
Note that it is a requirement to use the following VID and PID for the Switch to detect your controller:

| VID      | PID      |
| -------- | -------- |
| `0x0F0D` | `0x0092` |