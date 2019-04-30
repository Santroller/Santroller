#!/usr/bin/env python3

import usb.core
import serial
import sys
import time
import binascii
import ctypes

try:
    dev = usb.core.find(idVendor=0x1209, idProduct=0x2882)
    val = 0x31
    if dev is None:
        raise ValueError('Device not found')
    else:
            dev.ctrl_transfer(0x00, val, 0x00, 0x00)
except:
    pass

time.sleep(1)
ser = serial.Serial('/dev/ttyACM0', 115200, timeout=1)
ser.write(b'c')
data = ser.read(100)
for x in range(0, len(data)):
    print('{0:#0{1}x}'.format(data[x], 4))
