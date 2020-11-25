#!/usr/bin/env python
# import hid
# h = hid.device()
# h.open(0x1209, 0x2882)
# h.send_feature_report([0x00,52])
import time
import usb.core
import usb.util

# find our device
dev = usb.core.find(idVendor=0x1209, idProduct=0x2882)
try:
    dev.detach_kernel_driver(0)
except:
    print("Probably already detached")
print(dev.ctrl_transfer(0xa1, 0x01, 60,0x00,64))
time.sleep(1)
print(dev.ctrl_transfer(0xa1, 0x01, 61,0x00,64))
time.sleep(1)
print(dev.ctrl_transfer(0xa1, 0x01, 62,0x00,64))
time.sleep(1)
print(dev.ctrl_transfer(0xa1, 0x01, 63,0x00,64))
time.sleep(1)
print(dev.ctrl_transfer(0xa1, 0x01, 64,0x00,64))
time.sleep(1)
print(dev.ctrl_transfer(0xa1, 0x01, 65,0x00,64))