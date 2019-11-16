# Xbox360 Controller Identification and Capabilities
## Controller Identification
Xbox 360 controllers use a concept called [WCID](https://github.com/pbatard/libwdi/wiki/WCID-Devices) to identify to windows that they are 360 controllers. 

Wired controllers use a Compatible ID of "XUSB10", and Wireless controllers use a Compatible ID of "XUSB20". 
This is implemented in [Descriptors.c](/src/shared/lufa/Descriptors.c) and [XInputOutput.cpp](/src/shared/controller/output/XInputOutput.cpp). 

The libwdi github link above gives a lot of information about how to implement this, so I won't go into detail here.
## Capabilities
### Descriptor
Xbox 360 controllers have two steps for supporting custom capabilities. The first step is to have capability data in a usb descriptor of id 0x21.
This descriptor is laid out like the following:
```
 0                   1                   2                   3  
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|      0x00     |      0x00     |    Subtype    |      0x25     |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|      0x81     |      0x14     |      0x03     |      0x03     |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|      0x03     |      0x04     |      0x13     |      0x02     |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|      0x08     |      0x03     |      0x00     |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```
This is implemented in [Descriptors.c](../src/shared/output/usb/Descriptors.c). 

### The below requests are not necessary, but they are still documented below.
### Control Requests
Step two is to reply with capabilities packets when specific control requests are sent. These are laid out below:
#### ID
If a request with `bRequest` set to `HID_GetReport` (0x01) and with 'wLength' of 0x04 is received, and with `bmRequestType` set to `Device To Host, Vendor and Interface` then the following response is required:
```
 0                   1                   2                   3  
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|      0x00     |      0x12     |      0x28     |      0x61     |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

#### Capabilities 1
If a request with `bRequest` set to `HID_GetReport` (0x01) and with 'wLength' of 0x08 is received, and with `bmRequestType` set to `Device To Host, Vendor and Interface` then the following response is required:
```
 0                   1                   2                   3  
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|      0x00     |      0x08     |      0x00     |      0x00     |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|      0x00     |      0x00     |      0x00     |      0x00     |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

#### Capabilities 2
If a request with `bRequest` set to `HID_GetReport` (0x01) and with 'wLength' of 0x20 is received, and with `bmRequestType` set to `Device To Host, Vendor and Interface` then the following response is required:
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
The above information was taken from the following page: https://forums.vigem.org/topic/45/rogue-xinput-capabilities-bug-part-2.

A LUFA implementation of this controller is in [XInputOutput.cpp](/src/shared/output/output_xinput.c). 