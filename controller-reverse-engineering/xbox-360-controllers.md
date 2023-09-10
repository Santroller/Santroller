# Xbox 360 - Controller Identification and Capabilities
## Controller Identification
Xbox 360 controllers use a concept called [WCID](https://github.com/pbatard/libwdi/wiki/WCID-Devices) to identify to Windows that they are 360 controllers. 

Wired controllers use a Compatible ID of `XUSB10`, and Wireless controllers use a Compatible ID of `XUSB20`.

The link above gives a lot of information about how to implement this, so I won't go into detail here.
## Capabilities
Multi-byte fields below are little-endian unless otherwise specified.
### Descriptor
Xbox 360 controllers have two steps for supporting custom capabilities. The first step is to have capability data in a USB descriptor of type `0x21`.
This descriptor is laid out like the following:
```
 0                   1                   2                   3  
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|      0x10     |  Type (0x01)  |    Subtype    |      0x25     |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
| Input Endpoint| Length (0x14) |      0x03     |      0x03     |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|      0x03     |      0x04     |      0x13     |Output Endpoint|
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
| Length (0x08) |      0x03     |      0x03     |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

### Control Requests
Step two is to reply with capabilities packets when specific control requests are sent. These are laid out below:
#### Capabilities 1 (Vibration)
If a request with the following values is received:

- bmRequest: `0xc1`
  - Recipient: `0b_xxxx_xx01` (Interface)
  - Type: `0b_x10x_xxxx` (Vendor-defined)
  - Direction: `0b_1xxx_xxxx` (Device to host)
- bRequest: `0x01` (HID Get Report)
- wValue: `0x0000`
- wIndex: `0x0000`

Then a response detailing vibration capabilities is required:

```
 0                   1                   2                   3  
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|   RID (0x00)  | Length (0x08) |      0x00     |   Left Motor  |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|  Right Motor  |      0x00     |      0x00     |      0x00     |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

#### Serial Number
The console uses this serial number as a way to differenciate controllers so it needs to be different for the console to see two controllers as different controllers.
If a request with the following values is received:

- bmRequest: `0xc0`
  - Recipient: `0b_xxxx_xx00` (Device)
  - Type: `0b_x10x_xxxx` (Vendor-defined)
  - Direction: `0b_1xxx_xxxx` (Device to host)
- bRequest: `0x01` (HID Get Report)
- wValue: `0x0000`
- wIndex: `0x0000`

Then a response with the device's serial number is required:

```
 0                   1                   2                   3  
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                   Serial number (big-endian)                  |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

Examples:

```
 0                   1                   2                   3  
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|      0x00     |      0x12     |      0x28     |      0x61     |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

 0                   1                   2                   3  
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|      0x00     |      0x82     |      0xf8     |      0x23     |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

#### Capabilities 2 (Inputs)
If a request with the following values is received:

- bmRequest: `0xc1`
  - Recipient: `0b_xxxx_xx01` (Interface)
  - Type: `0b_x10x_xxxx` (Vendor-defined)
  - Direction: `0b_1xxx_xxxx` (Device to host)
- bRequest: `0x01` (HID Get Report)
- wValue: `0x0100`
- wIndex: `0x0000`

Then a response describing what inputs the device is capable of is required:

```
 0                   1                   2                   3  
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|   RID (0x00)  | Length (0x14) |            Buttons            |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
| Left Trigger  | Right Trigger |         Left Thumb X          |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|         Left Thumb Y          |         Right Thumb X         |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|         Right Thumb Y         |            Reserved           |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                               |             Flags             |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

Available flags:

- `0x0001`: Force feedback supported
- `0x0010`: No navigation buttons

Example:

```
 0                   1                   2                   3  
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|      0x00     |      0x14     |      0x3f     |      0xf7     |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|      0xff     |      0xff     |      0x00     |      0x00     |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|      0x00     |      0x00     |      0xc0     |      0xff     |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|      0xc0     |      0xff     |      0x00     |      0x00     |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|      0x00     |      0x00     |      0x00     |      0x00     |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

Some of these control requests were taken from the following site: https://forums.vigem.org/topic/45/rogue-xinput-capabilities-bug-part-2.

## Interesting Findings
If you don't send capabilities 1, then it is possible for a controller to crash DWM on windows.
Some Windows Store games come with [Windows Gaming Services](https://www.microsoft.com/en-us/p/gaming-services/9mwpm2cqnlhn) on Windows 10.
It seems that Windows Gaming Services is loaded into DWM by windows, and it has a bug where if capabilites 1 is missing, then it crashes, and it takes DWM with it. This will cause the Windows desktop to crash and restart.
