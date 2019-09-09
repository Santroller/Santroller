# PS3 Controller Breakdown
## HID report:
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
|               |        accelerometer_y        |accelerometer_z|
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|               |           gyroscope           |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```
### Axis Fields
The axis fields are there as all face buttons are actually pressure sensitive and can measure a value from 0x00 being not pressed, to 0xFF being fully pressed. Triggers are handled here too.
Accelerometer and gyroscope are 10 bit numbers

### Buttons
Buttons are broken up as follows, with one bit per button

```
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|    triangle   |     circle    |     cross     |     square    |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|       l2      |       r2      |       l1      |       r1      |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|     select    |     start     | left_stick_in | right_stick_in|
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|       ps      |
+-+-+-+-+-+-+-+-+
```

### POV Hat (DPad)
The hat values look like the below diagram. 0x08 is used when holding no buttons. So for example, holding both left and up results in a value of 0x01.

```
 7  0  1
  \ | /
6 - 8 - 2
  / | \
 5  4  3
```
## HID Report Descriptor:

```c
{
        0x05, 0x01, // Usage Page (Generic Desktop Ctrls)
        0x09, 0x05, // Usage (Game Pad)
        0xA1, 0x01, // Collection (Application)
        0x15, 0x00, //   Logical Minimum (0)
        0x25, 0x01, //   Logical Maximum (1)
        0x35, 0x00, //   Physical Minimum (0)
        0x45, 0x01, //   Physical Maximum (1)
        0x75, 0x01, //   Report Size (1)
        0x95, 0x0D, //   Report Count (13)
        0x05, 0x09, //   Usage Page (Button)
        0x19, 0x01, //   Usage Minimum (0x01)
        0x29, 0x0D, //   Usage Maximum (0x0D)
        0x81, 0x02, //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No
                    //   Null Position)
        0x95, 0x03, //   Report Count (3)
        0x81, 0x01, //   Input (Const,Array,Abs,No Wrap,Linear,Preferred
                    //   State,No Null Position)
        0x05, 0x01, //   Usage Page (Generic Desktop Ctrls)
        0x25, 0x07, //   Logical Maximum (7)
        0x46, 0x3B, 0x01, //   Physical Maximum (315)
        0x75, 0x04,       //   Report Size (4)
        0x95, 0x01,       //   Report Count (1)
        0x65, 0x14, //   Unit (System: English Rotation, Length: Centimeter)
        0x09, 0x39, //   Usage (Hat switch)
        0x81, 0x42, //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,Null
                    //   State)
        0x65, 0x00, //   Unit (None)
        0x95, 0x01, //   Report Count (1)
        0x81, 0x01, //   Input (Const,Array,Abs,No Wrap,Linear,Preferred
                    //   State,No Null Position)
        0x26, 0xFF, 0x00, //   Logical Maximum (255)
        0x46, 0xFF, 0x00, //   Physical Maximum (255)
        0x09, 0x30,       //   Usage (X)
        0x09, 0x31,       //   Usage (Y)
        0x09, 0x32,       //   Usage (Z)
        0x09, 0x35,       //   Usage (Rz)
        0x75, 0x08,       //   Report Size (8)
        0x95, 0x04,       //   Report Count (4)
        0x81, 0x02, //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No
                    //   Null Position)
        0x06, 0x00, 0xFF, //   Usage Page (Vendor Defined 0xFF00)
        0x09, 0x20,       //   Usage (0x20)
        0x09, 0x21,       //   Usage (0x21)
        0x09, 0x22,       //   Usage (0x22)
        0x09, 0x23,       //   Usage (0x23)
        0x09, 0x24,       //   Usage (0x24)
        0x09, 0x25,       //   Usage (0x25)
        0x09, 0x26,       //   Usage (0x26)
        0x09, 0x27,       //   Usage (0x27)
        0x09, 0x28,       //   Usage (0x28)
        0x09, 0x29,       //   Usage (0x29)
        0x09, 0x2A,       //   Usage (0x2A)
        0x09, 0x2B,       //   Usage (0x2B)
        0x95, 0x0C,       //   Report Count (12)
        0x81, 0x02, //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No
                    //   Null Position)
        0x0A, 0x21, 0x26, //   Usage (0x2621)
        0x95, 0x08,       //   Report Count (8)
        0xB1, 0x02, //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No
                    //   Null Position,Non-volatile)
        0x0A, 0x21, 0x26, //   Usage (0x2621)
        0x91, 0x02, //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No
                    //   Null Position,Non-volatile)
        0x26, 0xFF, 0x03, //   Logical Maximum (1023)
        0x46, 0xFF, 0x03, //   Physical Maximum (1023)
        0x09, 0x2C,       //   Usage (0x2C)
        0x09, 0x2D,       //   Usage (0x2D)
        0x09, 0x2E,       //   Usage (0x2E)
        0x09, 0x2F,       //   Usage (0x2F)
        0x75, 0x10,       //   Report Size (16)
        0x95, 0x04,       //   Report Count (4)
        0x81, 0x02, //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No
                    //   Null Position)
        0xC0,       // End Collection

};
```
## ID Control Request
For making the PS button work, there are some extra requirements
When the device receives a control request, with a `bmRequestType` of `device to host, class and interface` and a `bmRequest` of `HID_GET_REPORT (0x01)`, It needs to respond with the following bytes: 
```
 0                   1                   2                   3  
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|      0x21     |      0x26     |      0x01     |      0x07     |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|      0x00     |      0x00     |      0x00     |      0x00     |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

## VID and PID
Note that a specific VID and PID is not required at all for the controller to work.

A LUFA implementation of this controller is in [output_ps3.c](/src/shared/output/output_ps3.c)