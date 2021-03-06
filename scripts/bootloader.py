#!/usr/bin/env python3
# import hid
# h = hid.device()
# h.open(0x1209, 0x2882)
# h.send_feature_report([0x00,52])

import usb.core
import usb.util

# find our device
dev = usb.core.find(idVendor=0x1209, idProduct=0x2882)
try:
    dev.detach_kernel_driver(0)
except:
    print("Probably already detached")
try:
    dev.ctrl_transfer(0x21, 0x09, 49,0x00,[])
    dev.ctrl_transfer(0x21, 0x09, 49,0x02,[])
except:
    print("failed 1209")
dev = usb.core.find(idVendor=0x12ba, idProduct=0x0100)
try:
    dev.detach_kernel_driver(0)
except:
    print("Probably already detached")
try:
    dev.ctrl_transfer(0x21, 0x09, 49,0x00,[])
except:
    print("failed 1209")
dev = usb.core.find(idVendor=0x12ba, idProduct=0x0200)
try:
    dev.detach_kernel_driver(0)
except:
    print("Probably already detached")
try:
    dev.ctrl_transfer(0x21, 0x09, 49,0x00,[])
except:
    print("failed 1209")