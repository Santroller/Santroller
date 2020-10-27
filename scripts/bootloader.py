#!/usr/bin/env python
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
dev.ctrl_transfer(0x21, 0x09, 0x0300,0x00,[52])